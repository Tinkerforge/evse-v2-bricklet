/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * dc_fault.c: DC fault detection
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

#include "dc_fault.h"

#include "configs/config_dc_fault.h"
#include "configs/config_evse.h"

#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/warp/contactor_check.h"

#include "iec61851.h"
#include "adc.h"
#include "led.h"
#include "evse.h"
#include "communication.h"
#include "hardware_version.h"

#include "xmc_gpio.h"

DCFault dc_fault;

void dc_fault_init_pins(void) {
	const XMC_GPIO_CONFIG_t pin_config_output_high = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(DC_FAULT_TST_PIN, &pin_config_output_high);
	XMC_GPIO_Init(DC_FAULT_X6_PIN,  &pin_config_input);
	XMC_GPIO_Init(DC_FAULT_X30_PIN, &pin_config_input);
	XMC_GPIO_Init(DC_FAULT_ERR_PIN, &pin_config_input);
}

void dc_fault_init(void) {
	memset(&dc_fault, 0, sizeof(DCFault));
	dc_fault_init_pins();
}

void dc_fault_update_values(void) {
	static uint32_t t[3] = {0};

	// Don't update the dc fault values if the contactor is not turned on
	// It doesn't make any sense to check in this case, we can only get false positives
	if(XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN)) {
		return;
	}

	// Add an additional 1s timeout directly after the contactor is turned on/off
	if(!system_timer_is_time_elapsed_ms(evse.last_contactor_switch, 1000)) {
		return;
	}

	bool values[3] = {
		XMC_GPIO_GetInput(DC_FAULT_X6_PIN),
		XMC_GPIO_GetInput(DC_FAULT_X30_PIN),
		XMC_GPIO_GetInput(DC_FAULT_ERR_PIN)
	};

	for(uint8_t i = 0; i < 3; i++) {
		if(values[i]) {
			values[i] = false;
			if(t[i] != 0) {
				// The dc fault pin has to be logic high for at least 50ms before we accept it as high
				if(system_timer_is_time_elapsed_ms(t[i], 50)) {
					values[i] = true;
				}
			} else {
				t[i] = system_timer_get_ms();
			}
		} else {
			t[i] = 0;
			values[i] = false;
		}
	}

	dc_fault.x6    = values[0];
	dc_fault.x30   = values[1];
	dc_fault.error = values[2];
}

void dc_fault_calibration_reset(void) {
	dc_fault.calibration_start    = false;
	dc_fault.calibration_state    = 0;
	dc_fault.calibration_check[0] = false;
	dc_fault.calibration_check[1] = false;
	dc_fault.calibration_check[2] = false;
	dc_fault.x6                   = false;
	dc_fault.x30                  = false;
	dc_fault.error                = false;

	// Set calibration pins back to default
	dc_fault_init_pins();
}

void dc_fault_calibration_tick(void) {
	switch(dc_fault.calibration_state) {
		case 0: { // Pull TST low
			XMC_GPIO_SetOutputLow(DC_FAULT_TST_PIN);
			dc_fault.calibration_time = system_timer_get_ms();
			dc_fault.calibration_state++;
			break;
		}

		case 1: { // Wait for 250ms (between 30ms and 1.2s OK)
			if(system_timer_is_time_elapsed_ms(dc_fault.calibration_time, 250)) {
				XMC_GPIO_SetOutputHigh(DC_FAULT_TST_PIN);
				dc_fault.calibration_time = system_timer_get_ms();
				dc_fault.calibration_state++;
			}
			break;
		}

		case 2: { // Wait 740ms for 6x to go high
			if(system_timer_is_time_elapsed_ms(dc_fault.calibration_time, 740)) {
				dc_fault.calibration_check[0] = XMC_GPIO_GetInput(DC_FAULT_X6_PIN);
				dc_fault.calibration_time = system_timer_get_ms();
				dc_fault.calibration_state++;
			}
			break;
		}

		case 3: { // Wait 660ms for 30x to go high
			if(system_timer_is_time_elapsed_ms(dc_fault.calibration_time, 660)) {
				dc_fault.calibration_check[1] = XMC_GPIO_GetInput(DC_FAULT_X30_PIN);
				dc_fault.calibration_time = system_timer_get_ms();
				dc_fault.calibration_state++;
			}
			break;
		}

		case 4: { // Wait 400ms. With sensor X804 errror pin goes high, with sensor X904 error pin stays low
			if(system_timer_is_time_elapsed_ms(dc_fault.calibration_time, 400)) {
				dc_fault.sensor_type = XMC_GPIO_GetInput(DC_FAULT_ERR_PIN) ? DC_FAULT_SENSOR_X804 : DC_FAULT_SENSOR_X904;
				dc_fault.calibration_time = system_timer_get_ms();
				dc_fault.calibration_state++;
			}
		}

		case 5: { // Wait 1s for 6x and 30x to go low again
			// (datasheet says 2030 to 2100ms, we have 3000 in sum)
			if(system_timer_is_time_elapsed_ms(dc_fault.calibration_time, 1200)) {
				dc_fault.calibration_check[2] = (!XMC_GPIO_GetInput(DC_FAULT_X6_PIN)) && (!XMC_GPIO_GetInput(DC_FAULT_X30_PIN));

				if(dc_fault.calibration_check[0] && dc_fault.calibration_check[1] && dc_fault.calibration_check[2]) {
					// DC faul test and calibration OK
					if(led.state == LED_STATE_FLICKER) {
						// If we are during bootup turn of led flickering after calbration is finished
						led_set_on(false);
					}
				} else {
					// DC faul test/calibration not OK
					dc_fault.state = DC_FAULT_CALIBRATION;
					dc_fault.state |= (dc_fault.calibration_check[0] << 3) | (dc_fault.calibration_check[1] << 4) | (dc_fault.calibration_check[2] << 5);
					dc_fault.state |= dc_fault.sensor_type << 6;
				}

				dc_fault.calibration_running = false;
				dc_fault_calibration_reset();
			}
			break;
		}
	}
}

void dc_fault_tick(void) {
	// Run dc fault test at least every 24h
	if(system_timer_is_time_elapsed_ms(dc_fault.last_run_time, 1000*60*60*24)) {
		dc_fault.calibration_start = true;
	}

	if((dc_fault.state & 0b111) != DC_FAULT_NORMAL_CONDITION) {
		// In case of any dc fault error we don't run the dc fault code anymore.
		// We never want to accidentially reset back to normal condition
		// (only the user of the wallbox should be able to do this).
		return;
	}

	if(dc_fault.calibration_running) {
		dc_fault_calibration_tick();
		return;
	}

	if(dc_fault.calibration_start && (iec61851.state == IEC61851_STATE_A) && (adc_result.cp_pe_resistance > 0xFFFF) && XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN)) {
		dc_fault.calibration_running = true;
		dc_fault.last_run_time       = system_timer_get_ms();
		dc_fault_calibration_reset();
		return;
	}

	dc_fault_update_values();

	if(dc_fault.sensor_type == DC_FAULT_SENSOR_X804) {
		if(!dc_fault.x6 && !dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_NORMAL_CONDITION;
			dc_fault.last_fault_time = 0;
		} else if(dc_fault.x6 && dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_6MA_DC_AND_20MA_AC;
		} else if(dc_fault.x6 && !dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_6MA_DC;
		} else if(!dc_fault.x6 && dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_20MA_AC;
		} else if(dc_fault.x6 && dc_fault.x30 && dc_fault.error) {
			dc_fault.state = DC_FAULT_SYSTEM;
		} else {
			dc_fault.state = DC_FAULT_UNKOWN;
		}
	} else { // DC_FAULT_SENSOR_X904
		if(!dc_fault.x6 && !dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_NORMAL_CONDITION;
			dc_fault.last_fault_time = 0;
		} else if(dc_fault.x6 && dc_fault.x30 && !dc_fault.error) {
			dc_fault.state = DC_FAULT_6MA_DC;
		} else if(dc_fault.x6 && dc_fault.x30 && dc_fault.error) {
			dc_fault.state = DC_FAULT_SYSTEM;
		} else {
			dc_fault.state = DC_FAULT_UNKOWN;
		}
	}

	dc_fault.state |= (dc_fault.x6 << 3) | (dc_fault.x30 << 4) | (dc_fault.error << 5);
	dc_fault.state |= dc_fault.sensor_type << 6;

	if((dc_fault.state & 0b111) != DC_FAULT_NORMAL_CONDITION) {
		dc_fault.last_fault_time = system_timer_get_ms();

		// Ignore all ADC measurements for a while if the contactor is
		// switched on or off, to be sure that the resulting EMI spike does
		// not give us a wrong measurement.
		adc_ignore_results(8);

		// Also ignore contactor check for a while when contactor changes state
		contactor_check.invalid_counter = MAX(5, contactor_check.invalid_counter);

		// Contactor is normally only controlled in the iec61851 tick,
		// but in case of dc fault condition we turn the contactor off
		// no matter what state we are in or similar.
		XMC_GPIO_SetOutputHigh(EVSE_CONTACTOR_PIN);
		iec61851.state = IEC61851_STATE_EF;
	}
}
