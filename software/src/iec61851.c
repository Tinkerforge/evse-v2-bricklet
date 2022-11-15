/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iec61851.c: Implementation of IEC 61851 EVSE state machine
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

#include "iec61851.h"

#include <stdint.h>
#include <string.h>

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "configs/config_evse.h"
#include "adc.h"
#include "iec61851.h"
#include "lock.h"
#include "evse.h"
#include "contactor_check.h"
#include "led.h"
#include "button.h"
#include "dc_fault.h"
#include "communication.h"
#include "charging_slot.h"

// Resistance between CP/PE
// inf  Ohm -> no car present
// 2700 Ohm -> car present
//  880 Ohm -> car charging
//  240 Ohm -> car charging with ventilation
// ==>
// > 10000 -> State A
// >  1790 -> State B
// >  560 -> State C
// >  150 -> State D
// <  150 -> State E/F
#define IEC61851_CP_RESISTANCE_STATE_A 10000
#define IEC61851_CP_RESISTANCE_STATE_B  1790
#define IEC61851_CP_RESISTANCE_STATE_C   300
#define IEC61851_CP_RESISTANCE_STATE_D   150

// Resistance between PP/PE
// 1000..2200 Ohm => 13A
// 330..1000 Ohm  => 20A
// 150..330 Ohm   => 32A
// 75..150 Ohm    => 63A
#define IEC61851_PP_RESISTANCE_13A 1000
#define IEC61851_PP_RESISTANCE_20A  330
#define IEC61851_PP_RESISTANCE_32A  150


IEC61851 iec61851;

void iec61851_set_state(IEC61851State state) {
	if(state != iec61851.state) {
		// If we change the state from an error state to something else we save the time
		if(iec61851.state == IEC61851_STATE_EF) {
			iec61851.last_error_time = system_timer_get_ms();
		}

		// If we change to state C and we were in an error state before, we wait at least 30 seconds
		if((state == IEC61851_STATE_C) && (iec61851.last_error_time != 0)) {
			if(!system_timer_is_time_elapsed_ms(iec61851.last_error_time, 30*1000)) {
				return;
			}
			iec61851.last_error_time = 0;
		}

		// If we change to state C and the charging timer was not started, we start it
		if((state == IEC61851_STATE_C) && (evse.charging_time == 0)) {
			evse.charging_time = system_timer_get_ms();
		}

		if((state == IEC61851_STATE_A ) || (state == IEC61851_STATE_B)) {
			// Turn LED on with timer for standby if we have a state change to state A or B
			led_set_on(false);
		}

		if((iec61851.state != IEC61851_STATE_A) && (state == IEC61851_STATE_A)) {
			// If state changed from to A we invalidate the managed current
			// we have to handle the clear on dusconnect slots
			charging_slot_handle_disconnect();

			// If the charging timer is running and the car is disconnected, stop the charging timer
			evse.charging_time = 0;
		}

		iec61851.state             = state;
		iec61851.last_state_change = system_timer_get_ms();
	}
}

// TODO: We can find out that no cable is connected here
//       if resistance > 10000. Do we want to have a specific
//       state for that?
uint32_t iec61851_get_ma_from_pp_resistance(void) {
	if(adc_result.pp_pe_resistance >= 1000) {
		return 13000; // 13A
	} else if(adc_result.pp_pe_resistance >= 330) {
		return 20000; // 20A
	} else if(adc_result.pp_pe_resistance >= 150) {
		return 32000; // 32A
	} else {
		return 64000; // 64A
	}
}

uint32_t iec61851_get_max_ma(void) {
	return charging_slot_get_max_current();
}

// Duty cycle in pro mille (1/10 %)
float iec61851_get_duty_cycle_for_ma(uint32_t ma) {
	// Special case for managed mode.
	// In managed mode we support a temporary stop of charging without disconnecting the vehicle.
	if(ma == 0) {
		// 100% duty cycle => charging not allowed
		// we do 100% here instead of 0% (both mean charging not allowed)
		// to be able to still properly measure the resistance that the car applies.
		return 1000;
	}

	float duty_cycle;
	if(ma <= 51000) {
		duty_cycle = ma/60.0; // For 6A-51A: xA = %duty*0.6
	} else {
		duty_cycle = ma/250.0 + 640; // For 51A-80A: xA= (%duty - 64)*2.5
	}

	// The standard defines 8% as minimum and 100% as maximum
	return BETWEEN(80.0, duty_cycle, 1000.0);
}

void iec61851_state_a(void) {
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_state_b(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);
}

void iec61851_state_c(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
	led_set_breathing();
}

void iec61851_state_d(void) {
	// State D is not supported
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_state_ef(void) {
	// In case of error apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_tick(void) {
	if(dc_fault.state != DC_FAULT_NORMAL_CONDITION) {
		led_set_blinking(3);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if(contactor_check.error != 0) {
		led_set_blinking(4);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if(evse.config_jumper_current == EVSE_CONFIG_JUMPER_UNCONFIGURED) {
		// We don't allow the jumper to be unconfigured
		led_set_blinking(2);
		iec61851_set_state(IEC61851_STATE_EF);
	// For diode error check if
	// * We are in state B or C
	// * We see a negative voltage above -10V or a difference of >2V between with and without resistor
	// * The CP contact is connected
	// * We currently apply a PWM (i.e. max ma is not 0)
	} else if(((iec61851.state == IEC61851_STATE_B) || (iec61851.state == IEC61851_STATE_C)) && 
	          ((adc[0].result_mv[1] > -10000) || (ABS(adc[0].result_mv[1] - adc[1].result_mv[1]) > 2000)) &&
	          (evse_is_cp_connected()) &&
	          (iec61851_get_max_ma() != 0)) {
		// Wait for ADC CP/PE measurements to be valid
		if((adc[0].ignore_count > 0) || (adc[1].ignore_count > 0)) {
			return;
		}

		if(iec61851.diode_error_counter > 100) {
			led_set_blinking(5);
			iec61851.state = IEC61851_STATE_B;
			iec61851_state_b();
		} else {
			iec61851.diode_error_counter++;
		}
	} else {
		// Wait for ADC CP/PE measurements to be valid
		if((adc[0].ignore_count > 0) || (adc[1].ignore_count > 0)) {
			return;
		}

		// If we reach here the diode error was fixed and we can turn the blinking off again
		if(iec61851.diode_error_counter > 0) {
			iec61851.diode_error_counter--;
			if(iec61851.diode_error_counter == 0) {
				led_set_on(false);
			} else {
				return;
			}
		}

		// If the CP contact is disconnected we stay in the current IEC state, independend of the measured resistance
		// After CP contact was disconnected and is connected again, we wait for 500ms to make sure that ADC measurement is working again.
		if(evse_is_cp_connected() && ((iec61851.wait_after_cp_disconnect == 0) || system_timer_is_time_elapsed_ms(iec61851.wait_after_cp_disconnect, 500))) {
			iec61851.wait_after_cp_disconnect = 0;
			if(adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A) {
				iec61851_set_state(IEC61851_STATE_A);
			} else if(adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_B) {
				iec61851_set_state(IEC61851_STATE_B);
			} else if(adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_C) {
				if(charging_slot_get_max_current() == 0) {
					evse.charging_time = 0;
					iec61851_set_state(IEC61851_STATE_B);
				} else {
					iec61851_set_state(IEC61851_STATE_C);
				}
			} else if(adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_D) {
				led_set_blinking(5);
				iec61851_set_state(IEC61851_STATE_D);
			} else {
				led_set_blinking(5);
				iec61851_set_state(IEC61851_STATE_EF);
			}
		}
	}

	switch(iec61851.state) {
		case IEC61851_STATE_A:  iec61851_state_a();  break;
		case IEC61851_STATE_B:  iec61851_state_b();  break;
		case IEC61851_STATE_C:  iec61851_state_c();  break;
		case IEC61851_STATE_D:  iec61851_state_d();  break;
		case IEC61851_STATE_EF: iec61851_state_ef(); break;
	}
}

void iec61851_init(void) {
	memset(&iec61851, 0, sizeof(IEC61851));
	iec61851.last_state_change = system_timer_get_ms();
}

