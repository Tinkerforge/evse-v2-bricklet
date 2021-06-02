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

#include "configs/config_evse.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/bootloader/bootloader.h"

#include "adc.h"
#include "iec61851.h"
#include "lock.h"
#include "contactor_check.h"
#include "led.h"
#include "dc_fault.h"

#include "xmc_scu.h"
#include "xmc_ccu4.h"
#include "xmc_vadc.h"
#include "xmc_gpio.h"

#define EVSE_RELAY_MONOFLOP_TIME 10000 // 10 seconds

EVSE evse;

#define evse_cp_pwm_irq IRQ_Hdlr_30
void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) evse_cp_pwm_irq(void) {
	XMC_VADC_GLOBAL_BackgroundTriggerConversion(VADC);
	XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
}


void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor) {
	if(evse.last_cp_duty_cycle != cp_duty_cycle) {
		evse.last_cp_duty_cycle = cp_duty_cycle;

		adc_enable_all(cp_duty_cycle == 1000);
		ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 48000 - cp_duty_cycle*48);
	}

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

	if(((bool)!XMC_GPIO_GetInput(EVSE_RELAY_PIN)) != contactor) {
		// Ignore all ADC measurements for a while if the contactor is
		// switched on or off, to be sure that the resulting EMI spike does
		// not give us a wrong measurement.
		adc_ignore_results(4);

		// Also ignore contactor check for a while when contactor changes state
		contactor_check.invalid_counter = MAX(5, contactor_check.invalid_counter);

		if(contactor) {
			XMC_GPIO_SetOutputLow(EVSE_RELAY_PIN);
		} else {
			XMC_GPIO_SetOutputHigh(EVSE_RELAY_PIN);
		}
	}

#ifdef EVSE_LOCK_ENABLE
	if(!contactor) {
		if(lock_get_state() != LOCK_STATE_OPEN) {
			lock_set_locked(false);
		}
	}
#endif
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

	if(pin0 == 'h' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
	} else if(pin0 == 'o' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_6A;
	} else if(pin0 == 'l' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_10A;
	} else if(pin0 == 'h' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_13A;
	} else if(pin0 == 'o' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_16A;
	} else if(pin0 == 'l' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_20A;
	} else if(pin0 == 'h' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_25A;
	} else if(pin0 == 'o' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_32A;
	} else if(pin0 == 'l' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_SOFTWARE;
	} else {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
	}
}

// Use CCU40 slice 0 for CP/PE PWM on P1_0
// Use CCU40 slice 1 synchronously with slice 0 but with fixed duty-cycle
// Slice 1 is used to generate IRQ for ADC measurements to make sure that CP/PE PWM high level is measured at end of duty-cycle
void evse_init_cp_pwm(void) {
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

	// Set the period and compare register values
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC40, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(CCU40_CC41, EVSE_CP_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC40, 48000 - 0*48);
	XMC_CCU4_SLICE_SetTimerCompareMatch(CCU40_CC41, 48000 - 30*48); // 3% duty-cycle (this puts the ADC measurement right at the end of duty-cycle)

	XMC_CCU4_EnableShadowTransfer(CCU40, XMC_CCU4_SHADOW_TRANSFER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 | XMC_CCU4_SHADOW_TRANSFER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_1);

	XMC_GPIO_Init(EVSE_CP_PWM_PIN, &gpio_out_config);

	XMC_CCU4_EnableClock(CCU40, 0);
	XMC_CCU4_EnableClock(CCU40, 1);

	// Start CCU40 slice 0 and 1 in sync.
	XMC_CCU4_SLICE_ConfigureEvent(CCU40_CC40, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_ConfigureEvent(CCU40_CC41, XMC_CCU4_SLICE_EVENT_1, &event_config);
	XMC_CCU4_SLICE_StartConfig(CCU40_CC40, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_CCU4_SLICE_StartConfig(CCU40_CC41, XMC_CCU4_SLICE_EVENT_1, XMC_CCU4_SLICE_START_MODE_TIMER_START_CLEAR);
	XMC_SCU_SetCcuTriggerHigh(SCU_GENERAL_CCUCON_GSC40_Msk);

	// Enable interrupt for CP PWM (on slice 1)
	XMC_CCU4_SLICE_EnableEvent(CCU40_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP);
	XMC_CCU4_SLICE_SetInterruptNode(CCU40_CC41, XMC_CCU4_SLICE_IRQ_ID_COMPARE_MATCH_UP, XMC_CCU4_SLICE_SR_ID_2);

	// Interrupt for testing
//	NVIC_SetPriority(30, 1);
//	XMC_SCU_SetInterruptControl(30, XMC_SCU_IRQCTRL_CCU40_SR2_IRQ30);
//	NVIC_EnableIRQ(30);
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

	XMC_GPIO_Init(EVSE_RELAY_PIN,         &pin_config_output_high);
	XMC_GPIO_Init(EVSE_MOTOR_PHASE_PIN,   &pin_config_output_low);
	XMC_GPIO_Init(EVSE_CP_DISCONNECT_PIN, &pin_config_output_low);
	XMC_GPIO_Init(EVSE_OUTPUT_GP_PIN,     &pin_config_output_low);

	XMC_GPIO_Init(EVSE_MOTOR_INPUT_SWITCH_PIN, &pin_config_input);
	XMC_GPIO_Init(EVSE_INPUT_GP_PIN,           &pin_config_input);;

	evse_init_cp_pwm();

//	ccu4_pwm_init(EVSE_MOTOR_ENABLE_PIN, EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD-1); // 10 kHz
//	ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD);

	evse.config_jumper_current_software = 6000; // default software configuration is 6A
	evse.max_current_configured = 32000; // default user defined current ist 32A
	evse.charging_autostart = true;

	evse_init_jumper();
	evse_init_lock_switch();

	evse.startup_time = system_timer_get_ms();
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
		uartbb_printf("GPIO: Input %d, Output %d\n\r", XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN), XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN));
		uartbb_printf("Lock State: %d\n\r", lock.state);
	}
#endif
}

void evse_tick(void) {
	if(evse.startup_time != 0 && !system_timer_is_time_elapsed_ms(evse.startup_time, 1000)) {
		// Wait for 1s so everything can start/boot properly
		return;
	}

	// Turn LED on (LED flicker off after startup/calibration)
	if(evse.startup_time != 0) {
		evse.startup_time = 0;

		// Start a dc fault module calibration on startup
		dc_fault.calibration_start = true;

		// Return to make sure that the dc fault tick is called at least once before the iec61851 tick.
		return;
	}

	else if((evse.config_jumper_current == EVSE_CONFIG_JUMPER_SOFTWARE) || (evse.config_jumper_current == EVSE_CONFIG_JUMPER_UNCONFIGURED)) {
		led_set_blinking(2);
	} else if(dc_fault.calibration_running) {
		// TODO: Just do nothing here, right?
		// LED flickering here would just be confusing, this is not an error state.
	} else {
		// Otherwise we implement the EVSE according to IEC 61851.
		iec61851_tick();
	}

//	evse_tick_debug();
}
