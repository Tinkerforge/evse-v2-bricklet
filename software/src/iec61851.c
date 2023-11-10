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
#include "bricklib2/warp/contactor_check.h"
#include "configs/config_evse.h"
#include "adc.h"
#include "iec61851.h"
#include "lock.h"
#include "evse.h"
#include "led.h"
#include "button.h"
#include "dc_fault.h"
#include "communication.h"
#include "charging_slot.h"
#include "phase_control.h"

IEC61851 iec61851;

void iec61851_diode_error_reset(bool check_pending) {
	iec61851.diode_error_counter = 0;
	iec61851.diode_check_pending = check_pending;
	iec61851.diode_ok_counter = 0;
	iec61851.diode_vcp1_last_count = 0xFFFF;
	iec61851.diode_vcp2_last_count = 0xFFFF;
	iec61851.diode_vcp1_seen = 0;
	iec61851.diode_vcp2_seen = 0;
}

void iec61851_set_state(IEC61851State state) {
	if(state != iec61851.state) {
		// If we change from an error state to something else we save the time
		// If we then change to state C we wait at least 30 seconds
		// -> Don't start charging immediately after error
		if((iec61851.state == IEC61851_STATE_D) && (iec61851.last_error_time == 0)) {
			iec61851.last_error_time = system_timer_get_ms();
		}
		if(iec61851.state == IEC61851_STATE_EF) { // User has to disconnect first for error state EF
			iec61851.last_error_time = system_timer_get_ms();
		}
		if((state == IEC61851_STATE_C) && (iec61851.last_error_time != 0)) {
			if(!system_timer_is_time_elapsed_ms(iec61851.last_error_time, 30*1000)) {
				return;
			}
			iec61851.last_error_time = 0;
		}

		// If we change from state C to something else we save the time
		// If we then change back to state C we wait at least 5 seconds
		// -> Don't start charging immediately after charging was stopped
		if(iec61851.state == IEC61851_STATE_C) {
			iec61851.last_state_c_end_time = system_timer_get_ms();
		}
		if((state == IEC61851_STATE_C) && (iec61851.last_state_c_end_time != 0)) {
			if(!system_timer_is_time_elapsed_ms(iec61851.last_state_c_end_time, 5*1000)) {
				return;
			}
			iec61851.last_state_c_end_time = 0;
		}

		// If we change to state C and the charging timer was not started, we start
		if((state == IEC61851_STATE_C) && (evse.charging_time == 0)) {
			evse.charging_time = system_timer_get_ms();
		}

		if((state == IEC61851_STATE_A ) || (state == IEC61851_STATE_B)) {
			// Turn LED on with timer for standby if we have a state change to state A or B
			led_set_on(false);
		}

		if((iec61851.state != IEC61851_STATE_A) && (state == IEC61851_STATE_A)) {
			// If state changed from any non-A state to state A we invalidate the managed current
			// we have to handle the clear on disconnect slots
			charging_slot_handle_disconnect();

			// If the charging timer is running and the car is disconnected, stop the charging timer
			evse.charging_time = 0;

			// Start new dc fault test after each charging
			dc_fault.calibration_start = true;
		}

		// If we are in state A or in an error state, we will only allow to turn the contactor on after the next diode check.
		if(state == IEC61851_STATE_A || state == IEC61851_STATE_D || state == IEC61851_STATE_EF) {
			iec61851_diode_error_reset(true);
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

void iec61851_reset_ev_wakeup(void) {
	iec61851.state_b1b2_transition_time = 0;
	iec61851.state_b1b2_transition_seen = false;
}

void iec61851_handle_ev_wakeup(uint32_t ma) {
	// If we are in state b and we change from no PWM to PWM (0mA to >0mA)
	// then we start the b1b2 transition timer.
	if(iec61851.state_b1b2_transition_seen) {
		iec61851.state_b1b2_transition_time = system_timer_get_ms();
		iec61851.state_b1b2_transition_seen = false;
	}

	if(ma == 0) {
		iec61851_reset_ev_wakeup();
	}

	// EV wakeup handling according to IEC61851-1 Annex A.5.3
	if((iec61851.state_b1b2_transition_time != 0) && (!evse.control_pilot_disconnect)) {
		// Wait for 4 seconds for the EV to wake up
		if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 34*1000)) {
			if(XMC_GPIO_GetInput(EVSE_CP_DISCONNECT_PIN)) {
				iec61851.wait_after_cp_disconnect = system_timer_get_ms();
				adc_ignore_results(2);
				iec61851.currently_beeing_woken_up = false;
				XMC_GPIO_SetOutputLow(EVSE_CP_DISCONNECT_PIN);
			}
		}

		// Wait for 30 seconds for the EV to change resistance and IEC61851 state change to C
		// If this does not happen and ev wakeup is enabled we disconnect CP.
		else if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 30*1000)) {
			if(evse.ev_wakeup_enabled) {
				iec61851.currently_beeing_woken_up = true;
				XMC_GPIO_SetOutputHigh(EVSE_CP_DISCONNECT_PIN);
			}
		}
	}
}

void iec61851_state_a(void) {
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);

	iec61851_reset_ev_wakeup();
}

void iec61851_state_b(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);

	// If EV wakeup function is enabled, we handle the actual wakeup state machine in state B
	// (when EV is connected but not charging)
	iec61851_handle_ev_wakeup(ma);
}

void iec61851_state_c(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
	led_set_breathing();

	iec61851_reset_ev_wakeup();
}

void iec61851_state_d(void) {
	// State D is not supported
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);

	iec61851_reset_ev_wakeup();
}

void iec61851_state_ef(void) {
	// In case of error apply +12V to CP, disable contactor
	evse_set_output(1000, false);

	iec61851_reset_ev_wakeup();
}

void iec61851_tick(void) {
	if(hardware_version.is_v3 && (contactor_check.error & 1)) { // PE error should have highest priority
		led_set_blinking(4);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if((dc_fault.state & 0b111) != DC_FAULT_NORMAL_CONDITION) {
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
	          ((adc[ADC_CHANNEL_VCP1].result_mv[ADC_NEGATIVE_MEASUREMENT] > -10000) || (ABS(adc[ADC_CHANNEL_VCP1].result_mv[ADC_NEGATIVE_MEASUREMENT] - adc[ADC_CHANNEL_VCP2].result_mv[ADC_NEGATIVE_MEASUREMENT]) > 2000)) &&
	          (evse_is_cp_connected()) &&
	          (iec61851_get_max_ma() != 0)) {
		// Wait for ADC CP/PE measurements to be valid
		if((adc[ADC_CHANNEL_VCP1].ignore_count > 0) || (adc[ADC_CHANNEL_VCP2].ignore_count > 0)) {
			return;
		}

		if((iec61851.state == IEC61851_STATE_B) || (iec61851.diode_error_counter >= 100)) {
			// In state B set error counter to 100 immediately. We have to make sure that we don't activate the contactor, so we react immediately.
			// If the diode error does not persist anymore the counter will be decreased to 0 and then the error will be cleared.
			iec61851.diode_error_counter = 100;
			led_set_blinking(5);
			iec61851.state = IEC61851_STATE_B;
			iec61851_state_b();
		} else {
			// In state C we wait for at least 100 ticks to make sure that this is not just some kind of false positive glitch.
			iec61851.diode_error_counter++;
		}
	} else {
		// Wait for ADC CP/PE measurements to be valid
		if((adc[ADC_CHANNEL_VCP1].ignore_count > 0) || (adc[ADC_CHANNEL_VCP2].ignore_count > 0)) {
			return;
		}

		if((iec61851.state == IEC61851_STATE_B) || (iec61851.state == IEC61851_STATE_C)) {
			if(iec61851.diode_check_pending && (iec61851.diode_ok_counter > 100)) {
				iec61851_diode_error_reset(false);
			} else {
				// Check if we have seen three different adc counts before we start the OK counter.
				if((iec61851.diode_vcp1_seen < 3) || (iec61851.diode_vcp2_seen < 3)) {
					if(iec61851.diode_vcp1_last_count != adc[ADC_CHANNEL_VCP1].result_index[ADC_NEGATIVE_MEASUREMENT]) {
						iec61851.diode_vcp1_last_count = adc[ADC_CHANNEL_VCP1].result_index[ADC_NEGATIVE_MEASUREMENT];
						iec61851.diode_vcp1_seen++;
					}
					if(iec61851.diode_vcp2_last_count != adc[ADC_CHANNEL_VCP2].result_index[ADC_NEGATIVE_MEASUREMENT]) {
						iec61851.diode_vcp2_last_count = adc[ADC_CHANNEL_VCP2].result_index[ADC_NEGATIVE_MEASUREMENT];
						iec61851.diode_vcp2_seen++;
					}
				} else {
					iec61851.diode_ok_counter++;
				}
			}
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

		if(phase_control.in_progress) {
			phase_control_state_phase_change();
			return;
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
	iec61851_diode_error_reset(true);
}

