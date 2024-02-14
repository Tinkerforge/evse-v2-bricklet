/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * evse.c: EVSE implementation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "evse.h"

#include <float.h>

#include "configs/config_evse.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/warp/meter.h"
#include "bricklib2/warp/contactor_check.h"

#include "adc.h"
#include "iec61851.h"
#include "lock.h"
#include "led.h"
#include "dc_fault.h"
#include "communication.h"
#include "charging_slot.h"
#include "button.h"
#include "iec61851.h"
#include "hardware_version.h"

#include "xmc_scu.h"
#include "xmc_ccu4.h"
#include "xmc_vadc.h"
#include "xmc_gpio.h"

#define EVSE_RELAY_MONOFLOP_TIME 10000 // 10 seconds

EVSE evse;

// Interrupt for debugging
#if 0
#define evse_cp_pwm_irq IRQ_Hdlr_30
void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) evse_cp_pwm_irq(void) {
	XMC_VADC_GLOBAL_BackgroundTriggerConversion(VADC);
	XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
}
#endif


void evse_set_output(const float cp_duty_cycle, const bool contactor) {
	static int16_t last_resistance_counter = -2;
	evse_set_cp_duty_cycle(cp_duty_cycle);

	// If the contactor is to be enabled and the lock is currently
	// not completely closed, we start the locking procedure and return.
	// The contactor will only be enabled after the lock is closed.
#ifdef EVSE_LOCK_ENABLE
	if(contactor) {
		if(lock_get_state() != LOCK_STATE_CLOSE) {
			lock_set_locked(true);
			return;
		}
	}
#endif

	if(((bool)!XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN)) != contactor) {
		if(((cp_duty_cycle == 0) || (cp_duty_cycle == 1000)) && (!contactor)) {
			// If the duty cycle is set to either 0% or 100% PWM and the contactor is supposed to be turned off,
			// it is possible that the WARP Charger wants to turn off the charging session while the car
			// still wants to charge. In this case we wait until the car actually stops charging and
			// changes the resistance back to 2700 ohm before we turn the contactor off.
			// This assures that the contactor is not switched under load.
			//
			// NOTE: In case of an emergency (for example a dc fault detection) the contactor is of course switched off
			//       immediately and directly in the fault detection code without any regard to charging-state,
			//       PWM value, resistance or similar.
			//       This function is only called in non-emergency cases.

			if(adc_result.cp_pe_resistance <= IEC61851_CP_RESISTANCE_STATE_B) {
				if(evse.contactor_turn_off_time == 0) {
					evse.contactor_turn_off_time = system_timer_get_ms();
					return;
				} else if(system_timer_is_time_elapsed_ms(evse.contactor_turn_off_time, 3*1000)) {
					// The car has to respond within 3 seconds (see IEC 61851-1 standard table A.6 sequence 10.1),
					// thus after 3 seconds we turn the contactor off, even if the car has not yet responded yet.
					// In this case there may be some kind of communication error between wallbox and car and it
					// is better to turn the contactor off, even if still under load.
					evse.contactor_turn_off_time = 0;
				} else {
					return;
				}
			} else {
				evse.contactor_turn_off_time = 0;
			}
		}

		if(contactor) {
			// If we are asked to turn the contactor on, we first wait for the diode check to finish
			if(iec61851.diode_check_pending) {
				return;
			}

			// If we are asked to turn the contactor on, we want to see at least two adc measurements in a row
			// that supports this conclusion.
			// To do this the ADC code increaes a counter every time a new CP/PE resistance is saved.
			// We expect to see two consecutive measurements, otherwise we don't turn the contactor on.
			// This is a generic method to ignore glitches that persist for only one adc measurement
			if(((last_resistance_counter+1) & 0xFF) != adc_result.resistance_counter) {
				last_resistance_counter = adc_result.resistance_counter;
				return;
			}
		} else {
			last_resistance_counter = -2;
		}

		// Ignore all ADC measurements for a while if the contactor is
		// switched on or off, to be sure that the resulting EMI spike does
		// not give us a wrong measurement.
		adc_ignore_results(4);

		// Also ignore contactor check for a while when contactor changes state
		contactor_check.invalid_counter = MAX(5, contactor_check.invalid_counter);

		if(contactor) {
			XMC_GPIO_SetOutputLow(EVSE_CONTACTOR_PIN);
		} else {
			XMC_GPIO_SetOutputHigh(EVSE_CONTACTOR_PIN);
		}

		evse.last_contactor_switch = system_timer_get_ms();
	}

#ifdef EVSE_LOCK_ENABLE
	if(!contactor) {
		if(lock_get_state() != LOCK_STATE_OPEN) {
			lock_set_locked(false);
		}
	}
#endif
}

void evse_load_config(void) {
	uint32_t page[EEPROM_PAGE_SIZE/sizeof(uint32_t)];
	bootloader_read_eeprom_page(EVSE_CONFIG_PAGE, page);

	// The magic number is not where it is supposed to be.
	// This is either our first startup or something went wrong.
	// We initialize the config data with sane default values.
	if(page[EVSE_CONFIG_MAGIC_POS] != EVSE_CONFIG_MAGIC) {
		evse.legacy_managed               = false;
		meter.relative_energy_sum.f       = 0.0f;
		evse.shutdown_input_configuration = EVSE_V2_SHUTDOWN_INPUT_IGNORED;
	} else {
		evse.legacy_managed               = page[EVSE_CONFIG_MANAGED_POS];
		meter.relative_energy_sum.data    = page[EVSE_CONFIG_REL_SUM_POS];
		evse.shutdown_input_configuration = page[EVSE_CONFIG_SHUTDOWN_INPUT_POS];
	}

	if(page[EVSE_CONFIG_MAGIC2_POS] != EVSE_CONFIG_MAGIC2) {
		evse.input_configuration          = 0;
		evse.output_configuration         = EVSE_V2_OUTPUT_HIGH_IMPEDANCE;
		button.configuration              = EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING;
	} else {
		evse.input_configuration          = page[EVSE_CONFIG_INPUT_POS];
		evse.output_configuration         = page[EVSE_CONFIG_OUTPUT_POS];
		button.configuration              = page[EVSE_CONFIG_BUTTON_POS];
	}

	if(page[EVSE_CONFIG_MAGIC3_POS] != EVSE_CONFIG_MAGIC3) {
		evse.boost_mode_enabled           = false;
		evse.ev_wakeup_enabled            = true;
	} else {
		evse.boost_mode_enabled           = page[EVSE_CONFIG_BOOST_POS];
		evse.ev_wakeup_enabled            = page[EVSE_CONFIG_EV_WAKUEP_POS];
	}

	if(page[EVSE_CONFIG_MAGIC4_POS] != EVSE_CONFIG_MAGIC4) {
		meter.relative_energy_import.f    = 0.0f;
		meter.relative_energy_export.f    = 0.0f;
	} else {
		meter.relative_energy_import.data = page[EVSE_CONFIG_REL_IMPORT_POS];
		meter.relative_energy_export.data = page[EVSE_CONFIG_REL_EXPORT_POS];
	}

	// Handle charging slot defaults
	EVSEChargingSlotDefault *slot_default = (EVSEChargingSlotDefault *)(&page[EVSE_CONFIG_SLOT_DEFAULT_POS]);
	if(slot_default->magic == EVSE_CONFIG_SLOT_MAGIC) {
		for(uint8_t i = 0; i < 18; i++) {
			charging_slot.max_current_default[i]         = slot_default->current[i];
			charging_slot.active_default[i]              = slot_default->active_clear[i] & 1;
			charging_slot.clear_on_disconnect_default[i] = slot_default->active_clear[i] & 2;
		}
	} else {
		// If there is no default the button slot is activated and everything else is deactivated
		for(uint8_t i = 0; i < 18; i++) {
			charging_slot.max_current_default[i]         = 32000;
			charging_slot.active_default[i]              = false;
			charging_slot.clear_on_disconnect_default[i] = false;
		}

		// Those are default indices, _not_ slot indices.
		charging_slot.max_current_default[2]         = 32000;
		charging_slot.active_default[2]              = true;
		charging_slot.clear_on_disconnect_default[2] = false;

		charging_slot.max_current_default[5]         = 0;
		charging_slot.active_default[5]              = evse.legacy_managed;
		charging_slot.clear_on_disconnect_default[5] = evse.legacy_managed;
	}

	logd("Load config:\n\r");
	logd(" * legacy managed    %d\n\r", evse.legacy_managed);
	logd(" * relener           %d %d %d\n\r", meter.relative_energy_sum.data, meter.relative_energy_import.data, meter.relative_energy_export.data);
	logd(" * shutdown input    %d\n\r", evse.shutdown_input_configuration);
	logd(" * slot current      %d %d %d %d %d %d %d %d", charging_slot.max_current_default[0], charging_slot.max_current_default[1], charging_slot.max_current_default[2], charging_slot.max_current_default[3], charging_slot.max_current_default[4], charging_slot.max_current_default[5], charging_slot.max_current_default[6], charging_slot.max_current_default[7]);
	logd(" * slot active/clear %d %d %d %d %d %d %d %d", charging_slot.clear_on_disconnect_default[0], charging_slot.clear_on_disconnect_default[1], charging_slot.clear_on_disconnect_default[2], charging_slot.clear_on_disconnect_default[3], charging_slot.clear_on_disconnect_default[4], charging_slot.clear_on_disconnect_default[5], charging_slot.clear_on_disconnect_default[6], charging_slot.clear_on_disconnect_default[7]);
}

void evse_save_config(void) {
	uint32_t page[EEPROM_PAGE_SIZE/sizeof(uint32_t)];

	page[EVSE_CONFIG_MAGIC_POS]           = EVSE_CONFIG_MAGIC;
	page[EVSE_CONFIG_MANAGED_POS]         = evse.legacy_managed;
	page[EVSE_CONFIG_SHUTDOWN_INPUT_POS]  = evse.shutdown_input_configuration;
	if(meter.reset_energy_meter) {
		page[EVSE_CONFIG_REL_SUM_POS]     = meter_register_set.total_kwh_sum.data;
		page[EVSE_CONFIG_REL_IMPORT_POS]  = meter_register_set.total_import_kwh.data;
		page[EVSE_CONFIG_REL_EXPORT_POS]  = meter_register_set.total_export_kwh.data;
		meter.relative_energy_sum.data    = meter_register_set.total_kwh_sum.data;
		meter.relative_energy_import.data = meter_register_set.total_import_kwh.data;
		meter.relative_energy_export.data = meter_register_set.total_export_kwh.data;
	} else {
		page[EVSE_CONFIG_REL_SUM_POS]     = meter.relative_energy_sum.data;
		page[EVSE_CONFIG_REL_IMPORT_POS]  = meter.relative_energy_import.data;
		page[EVSE_CONFIG_REL_EXPORT_POS]  = meter.relative_energy_export.data;
	}
	meter.reset_energy_meter = false;

	page[EVSE_CONFIG_MAGIC2_POS]          = EVSE_CONFIG_MAGIC2;
	page[EVSE_CONFIG_INPUT_POS]           = evse.input_configuration;
	page[EVSE_CONFIG_OUTPUT_POS]          = evse.output_configuration;
	page[EVSE_CONFIG_BUTTON_POS]          = button.configuration;

	page[EVSE_CONFIG_MAGIC3_POS]          = EVSE_CONFIG_MAGIC3;
	page[EVSE_CONFIG_BOOST_POS]           = evse.boost_mode_enabled;
	page[EVSE_CONFIG_EV_WAKUEP_POS]       = evse.ev_wakeup_enabled;

	page[EVSE_CONFIG_MAGIC4_POS]          = EVSE_CONFIG_MAGIC4;

	// Handle charging slot defaults
	EVSEChargingSlotDefault *slot_default = (EVSEChargingSlotDefault *)(&page[EVSE_CONFIG_SLOT_DEFAULT_POS]);
	for(uint8_t i = 0; i < 18; i++) {
		slot_default->current[i]      = charging_slot.max_current_default[i];
		slot_default->active_clear[i] = (charging_slot.active_default[i] << 0) | (charging_slot.clear_on_disconnect_default[i] << 1);
	}
	slot_default->magic = EVSE_CONFIG_SLOT_MAGIC;

	bootloader_write_eeprom_page(EVSE_CONFIG_PAGE, page);
}

void evse_factory_reset(void) {
	uint32_t page[EEPROM_PAGE_SIZE/sizeof(uint32_t)] = {0};
	bootloader_write_eeprom_page(EVSE_CONFIG_PAGE, page);

	NVIC_SystemReset();
}

uint16_t evse_get_cp_pwm_duty_cycle(void) {
	return XMC_CCU4_SLICE_GetTimerCompareMatch(hardware_version.is_v2 ? CCU40_CC40 : CCU41_CC40);
}

uint16_t evse_get_cp_duty_cycle(void) {
	uint16_t duty_cycle = (uint16_t)((48000 - evse_get_cp_pwm_duty_cycle())/48.0 + 0.5);
	if((duty_cycle >= EVSE_BOOST_MODE_US) && (duty_cycle != 1000) && evse.boost_mode_enabled) {
		return duty_cycle - EVSE_BOOST_MODE_US;
	}

	return duty_cycle;
}

void evse_set_cp_duty_cycle(const float duty_cycle) {
	static float last_duty_cycle = FLT_MAX;
	if(((last_duty_cycle == 0) || (last_duty_cycle == 1000)) && ((duty_cycle > 0) && (duty_cycle < 1000))) {
		iec61851.state_b1b2_transition_seen = true;
	}
	last_duty_cycle = duty_cycle;

	// According to IEC 61841-1 table A2 the duty cycle is allowed to be off by up to 5us.
	// If boost mode is enabled we add 4us to the duty cycle. This means that we are still within the standard.
	uint16_t adc_boost = 0;
	if((duty_cycle != 0) && (duty_cycle != 1000) && evse.boost_mode_enabled) {
		adc_boost = EVSE_BOOST_MODE_US;
	}

	const uint16_t current_cp_duty_cycle = evse_get_cp_pwm_duty_cycle();
	const uint16_t new_cp_duty_cycle     = (uint16_t)(48000 - (duty_cycle + adc_boost)*48 + 0.5);

	if(current_cp_duty_cycle != new_cp_duty_cycle) {
		adc_enable_all(duty_cycle > 999.99);

		// EVSE V2 uses CCU40, EVSE V3 uses CCU41
		XMC_CCU4_SLICE_SetTimerCompareMatch(hardware_version.is_v2 ? CCU40_CC40 : CCU41_CC40, new_cp_duty_cycle);
		XMC_CCU4_EnableShadowTransfer(hardware_version.is_v2 ? CCU40 : CCU41, XMC_CCU4_SHADOW_TRANSFER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0);
	}
}

// TODO: For now we don't support lock switch
void evse_init_lock_switch(void) {
	evse.has_lock_switch = false;
}

// Check pin header for max current
void evse_init_jumper(void) {
	const XMC_GPIO_CONFIG_t pin_config_input_tristate = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	const XMC_GPIO_CONFIG_t pin_config_input_pullup = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	const XMC_GPIO_CONFIG_t pin_config_input_pulldown = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_DOWN,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_pullup);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_pullup);
	system_timer_sleep_ms(50);
	bool pin0_pu = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN0);
	bool pin1_pu = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN1);

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_pulldown);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_pulldown);
	system_timer_sleep_ms(50);
	bool pin0_pd = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN0);
	bool pin1_pd = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN1);

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_tristate);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_tristate);

	// Differentiate between high, low and open
	char pin0 = 'x';
	if(pin0_pu && !pin0_pd) {
		pin0 = 'o';
	} else if(pin0_pu && pin0_pd) {
		pin0 = 'h';
	} else if(!pin0_pu && !pin0_pd) {
		pin0 = 'l';
	}

	char pin1 = 'x';
	if(pin1_pu && !pin1_pd) {
		pin1 = 'o';
	} else if(pin1_pu && pin1_pd) {
		pin1 = 'h';
	} else if(!pin1_pu && !pin1_pd) {
		pin1 = 'l';
	}

	if(hardware_version.is_v2) {
		if(pin0 == 'h' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_RESERVED;
		} else if(pin0 == 'o' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_32A;
		} else if(pin0 == 'l' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_25A;
		} else if(pin0 == 'h' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_20A;
		} else if(pin0 == 'o' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_16A;
		} else if(pin0 == 'l' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_13A;
		} else if(pin0 == 'h' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_10A;
		} else if(pin0 == 'o' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_6A;
		} else if(pin0 == 'l' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
		} else {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
		}
	} else { // V3
		if(pin0 == 'h' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_32A;
		} else if(pin0 == 'o' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_RESERVED;
		} else if(pin0 == 'l' && pin1 == 'h') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_25A;
		} else if(pin0 == 'h' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_13A;
		} else if(pin0 == 'o' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
		} else if(pin0 == 'l' && pin1 == 'o') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_6A;
		} else if(pin0 == 'h' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_20A;
		} else if(pin0 == 'o' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_10A;
		} else if(pin0 == 'l' && pin1 == 'l') {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_16A;
		} else {
			evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
		}
	}
}

// Use CCU40 slice 0 for CP/PE
// Use CCU40 slice 1 synchronously with slice 0 but with fixed duty-cycle
// Slice 1 is used to generate IRQ for ADC measurements to make sure that CP/PE PWM high level is measured at end of duty-cycle
void evse_v2_init_cp_pwm(void) {
	const XMC_CCU4_SLICE_COMPARE_CONFIG_t compare_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = 0,
		.prescaler_initval   = XMC_CCU4_SLICE_PRESCALER_2,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = 0
	};

 	const XMC_CCU4_SLICE_EVENT_CONFIG_t event_config = {
		 .duration = XMC_CCU4_SLICE_EVENT_FILTER_5_CYCLES,
		 .edge = XMC_CCU4_SLICE_EVENT_EDGE_SENSITIVITY_RISING_EDGE,
		 .level = XMC_CCU4_SLICE_EVENT_LEVEL_SENSITIVITY_ACTIVE_HIGH,
		 .mapped_input = XMC_CCU4_SLICE_INPUT_AI
	};

	const XMC_GPIO_CONFIG_t gpio_out_config	= {
		.mode                = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

	XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU4_StartPrescaler(CCU40);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC40, &compare_config);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC41, &compare_config);
	XMC_CCU4_SLICE_CompareInit(CCU40_CC42, &compare_config);

	// Set the period and compare register values
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC40, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC41, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC42, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC40, 48000 - 0*48);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC41, 48000 - 30*48); // 3% duty-cycle (this puts the first ADC measurement at the end of the high part of the PWM)
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC42, 48000 - 750*48); // 75% duty-cycle (this puts the second ADC measurement at the middle the low part of the PWM)

	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_SLICE_2 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_2);

	XMC_GPIO_Init(EVSE_CP_PWM_PIN, &gpio_out_config);

	XMC_CCU4_EnableClock(CCU40, 0);
	XMC_CCU4_EnableClock(CCU40, 1);
	XMC_CCU4_EnableClock(CCU40, 2);

	// Start CCU40 slice 0, 1 and 2 in sync.
	XMC_CCU4_SLICE_ConfigureEvent(CCU40_CC40, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_ConfigureEvent(CCU40_CC41, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_ConfigureEvent(CCU40_CC42, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_StartConfig(CCU40_CC40, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_CCU4_SLICE_StartConfig(CCU40_CC41, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_CCU4_SLICE_StartConfig(CCU40_CC42, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_SCU_SetCcuTriggerHigh(SCU_GENERAL_CCUCON_GSC40_Msk);

	// Enable interrupt for CP PWM (on slice 1)
	XMC_CCU4_SLICE_EnableEvent(CCU40_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
	XMC_CCU4_SLICE_SetInterruptNode(CCU40_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP, XMC_CCU4_SLICE_SR_ID_2);

	// Enable interrupt for CP PWM (on slice 2)
	XMC_CCU4_SLICE_EnableEvent(CCU40_CC42, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
	XMC_CCU4_SLICE_SetInterruptNode(CCU40_CC42, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP, XMC_CCU4_SLICE_SR_ID_2);

	evse_set_cp_duty_cycle(1000.0);

	// Interrupt for debugging, uncomment if needed
//	NVIC_SetPriority(30, 1);
//	XMC_SCU_SetInterruptControl(30, XMC_SCU_IRQCTRL_CCU40_SR2_IRQ30);
//	NVIC_EnableIRQ(30);
}

void evse_v3_init_cp_pwm(void) {
	const XMC_CCU4_SLICE_COMPARE_CONFIG_t compare_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = 0,
		.prescaler_initval   = XMC_CCU4_SLICE_PRESCALER_2,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = 0
	};

 	const XMC_CCU4_SLICE_EVENT_CONFIG_t event_config = {
		 .duration = XMC_CCU4_SLICE_EVENT_FILTER_5_CYCLES,
		 .edge = XMC_CCU4_SLICE_EVENT_EDGE_SENSITIVITY_RISING_EDGE,
		 .level = XMC_CCU4_SLICE_EVENT_LEVEL_SENSITIVITY_ACTIVE_HIGH,
		 .mapped_input = XMC_CCU4_SLICE_INPUT_AI
	};

	const XMC_GPIO_CONFIG_t gpio_out_config	= {
		.mode                = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

	XMC_CCU4_Init(CCU41, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU4_StartPrescaler(CCU41);
	XMC_CCU4_SLICE_CompareInit(CCU41_CC40, &compare_config);
	XMC_CCU4_SLICE_CompareInit(CCU41_CC41, &compare_config);
	XMC_CCU4_SLICE_CompareInit(CCU41_CC42, &compare_config);

	// Set the period and compare register values
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU41_CC40, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU41_CC41, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU41_CC42, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU41_CC40, 48000 - 0*48);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU41_CC41, 48000 - 30*48); // 3% duty-cycle (this puts the first ADC measurement at the end of the high part of the PWM)
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU41_CC42, 48000 - 750*48); // 75% duty-cycle (this puts the second ADC measurement at the middle the low part of the PWM)

	XMC_CCU4_EnableShadowTransfer(CCU41, XMC_CCU4_SHADOW_TRANSFER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_SLICE_2 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_2);

	XMC_GPIO_Init(EVSE_CP_PWM_PIN, &gpio_out_config);

	XMC_CCU4_EnableClock(CCU41, 0);
	XMC_CCU4_EnableClock(CCU41, 1);
	XMC_CCU4_EnableClock(CCU41, 2);

	// Start CCU40 slice 0, 1 and 2 in sync.
	XMC_CCU4_SLICE_ConfigureEvent(CCU41_CC40, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_ConfigureEvent(CCU41_CC41, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_ConfigureEvent(CCU41_CC42, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_StartConfig(CCU41_CC40, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_CCU4_SLICE_StartConfig(CCU41_CC41, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_CCU4_SLICE_StartConfig(CCU41_CC42, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_SCU_SetCcuTriggerHigh(SCU_GENERAL_CCUCON_GSC41_Msk);

	// Enable interrupt for CP PWM (on slice 1)
	XMC_CCU4_SLICE_EnableEvent(CCU41_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
	XMC_CCU4_SLICE_SetInterruptNode(CCU41_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP, XMC_CCU4_SLICE_SR_ID_2);

	// Enable interrupt for CP PWM (on slice 2)
	XMC_CCU4_SLICE_EnableEvent(CCU41_CC42, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
	XMC_CCU4_SLICE_SetInterruptNode(CCU41_CC42, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP, XMC_CCU4_SLICE_SR_ID_2);

	evse_set_cp_duty_cycle(1000.0);

	// Interrupt for debugging, uncomment if needed
//	NVIC_SetPriority(30, 1);
//	XMC_SCU_SetInterruptControl(30, XMC_SCU_IRQCTRL_CCU40_SR2_IRQ30);
//	NVIC_EnableIRQ(30);
}

bool evse_is_shutdown(void) {
	// TODO: Debounce?

	if(evse.shutdown_input_configuration == EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_CLOSE) {
		if(!XMC_GPIO_GetInput(EVSE_SHUTDOWN_PIN)) {
			return true;
		}
	} else if(evse.shutdown_input_configuration == EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_OPEN) {
		if(XMC_GPIO_GetInput(EVSE_SHUTDOWN_PIN)) {
			return true;
		}
	}

	return false;
}

bool evse_is_cp_connected(void) {
	return !XMC_GPIO_GetInput(EVSE_CP_DISCONNECT_PIN);
}

void evse_init(void) {
	const XMC_GPIO_CONFIG_t pin_config_output_low = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	const XMC_GPIO_CONFIG_t pin_config_output_high = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_CONTACTOR_PIN, &pin_config_output_high);
	XMC_GPIO_Init(EVSE_CP_DISCONNECT_PIN, &pin_config_output_low);

	XMC_GPIO_Init(EVSE_SHUTDOWN_PIN,      &pin_config_input);

	if(hardware_version.is_v2) {
		evse_v2_init_cp_pwm();
		// Support for lock switch motor and input pin only in EVSE V2 
		XMC_GPIO_Init(EVSE_INPUT_GP_PIN,           &pin_config_input);
		XMC_GPIO_Init(EVSE_MOTOR_PHASE_PIN,        &pin_config_output_low);
		XMC_GPIO_Init(EVSE_MOTOR_INPUT_SWITCH_PIN, &pin_config_input);
//		ccu4_pwm_init(EVSE_MOTOR_ENABLE_PIN, EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD-1); // 10 kHz
//		ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD);
	} else if(hardware_version.is_v3) {
		evse_v3_init_cp_pwm();
	}

	evse.last_contactor_switch = system_timer_get_ms();
	evse.output_configuration = EVSE_V2_OUTPUT_HIGH_IMPEDANCE;
	evse.control_pilot_disconnect = false;
	evse.boost_mode_enabled = false;
	evse.ev_wakeup_enabled = true;

	evse_load_config();

	// Support for gp output pin only in EVSE V2
	if(hardware_version.is_v2) {
		if(evse.output_configuration == EVSE_V2_OUTPUT_CONNECTED_TO_GROUND) {
			XMC_GPIO_Init(EVSE_OUTPUT_GP_PIN, &pin_config_output_high);
		} else if(evse.output_configuration == EVSE_V2_OUTPUT_HIGH_IMPEDANCE) {
			XMC_GPIO_Init(EVSE_OUTPUT_GP_PIN, &pin_config_output_low);
		}
	}

	evse_init_jumper();
	evse_init_lock_switch();

	evse.startup_time = system_timer_get_ms();
	evse.charging_time = 0;
	evse.factory_reset_time = 0;
	evse.communication_watchdog_time = 0;
	evse.contactor_turn_off_time = 0;
}

void evse_tick_debug(void) {
#if LOGGING_LEVEL != LOGGING_NONE
	static uint32_t debug_time = 0;
	if(system_timer_is_time_elapsed_ms(debug_time, 250)) {
		debug_time = system_timer_get_ms();
		uartbb_printf("\n\r");
		uartbb_printf("IEC61851 State: %d\n\r", iec61851.state);
		uartbb_printf("Has lock switch: %d\n\r", evse.has_lock_switch);
		uartbb_printf("Jumper configuration: %d\n\r", evse.config_jumper_current);
		uartbb_printf("LED State: %d\n\r", led.state);
		uartbb_printf("Resistance: CP %d, PP %d\n\r", adc_result.cp_pe_resistance, adc_result.pp_pe_resistance);
		uartbb_printf("CP PWM duty cycle: %d\n\r", ccu4_pwm_get_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER));
		uartbb_printf("Contactor Check: AC1 %d, AC2 %d, State: %d, Error: %d\n\r", contactor_check.ac1_edge_count, contactor_check.ac2_edge_count, contactor_check.state, contactor_check.error);
		if(hardware_version.is_v2) {
			uartbb_printf("GPIO: Input %d, Output %d\n\r", XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN), XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN));
		}
		uartbb_printf("Lock State: %d\n\r", lock.state);
	}
#endif
}

void evse_tick(void) {
	if(evse.startup_time != 0 && !system_timer_is_time_elapsed_ms(evse.startup_time, 1000)) {
		// Wait for 1s so everything can start/boot properly
		return;
	}

	if(evse.factory_reset_time != 0) {
		if(system_timer_is_time_elapsed_ms(evse.factory_reset_time, 500)) {
			evse_factory_reset();
		}
	}

	// Turn LED on (LED flicker off after startup/calibration)
	if(evse.startup_time != 0) {
		evse.startup_time = 0;

		// Start a dc fault module calibration on startup
		dc_fault.calibration_start = true;

		// Return to make sure that the dc fault tick is called at least once before the iec61851 tick.
		return;
	} else if(dc_fault.calibration_running) {
		// TODO: Just do nothing here, right?
		// LED flickering here would just be confusing, this is not an error state.
	} else {
		// Otherwise we implement the EVSE according to IEC 61851.
		iec61851_tick();
	}

	// Restart EVSE after 5 minutes without any communication with a Brick
	if((evse.communication_watchdog_time != 0) && system_timer_is_time_elapsed_ms(evse.communication_watchdog_time, 1000*60*5)) {
		// Only restart EVSE if brick-communication-watchdog triggers if no car is connected
		if(iec61851.state == IEC61851_STATE_A) {
			NVIC_SystemReset();
		}
	}

//	evse_tick_debug();
}
