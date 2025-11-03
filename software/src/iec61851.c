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

// Resistance between CP/PE
// inf  Ohm -> no ev present
// 2700 Ohm -> ev present
//  880 Ohm -> ev charging
//  240 Ohm -> ev charging with ventilation
// ==>
// > 10000 -> State A
// >  1790 -> State B
// >  300 -> State C
// >  150 -> State D
// <  150 -> State E/F
// #define IEC61851_CP_RESISTANCE_STATE_A 10000
// #define IEC61851_CP_RESISTANCE_STATE_B  1790
// #define IEC61851_CP_RESISTANCE_STATE_C   300
// #define IEC61851_CP_RESISTANCE_STATE_D   150

// CP resistance for state A, B, C, D and E/F with 10% hysteresis between states.
// This hysteresis is made to conform to the IEC 61851-1 A.4.11 "Optional hysteresis test"
const uint16_t cp_resistance_state[5][4] = {
	// State A (ev not connected) transition to
	{
		9000,  // state A (10000 -10%)
		1790,  // state B
		300,   // state C
		150    // state D
	},
	// State B (ev connected) transition to
	{
		11000, // state A (10000 +10%)
		1611,  // state B (1790  -10%)
		300,   // state C
		150    // state D
	},
	// State C (ev wants charge) transition to
	{
		10000, // state A
		1969,  // state B (1790  +10%)
		270,   // state C (300   -10%)
		150    // state D
	},
	// State D (ev wants ventilation (not supported)) transition to
	{
		10000, // state A
		1790,  // state B
		330,   // state C (300   +10%)
		135    // state D (150   -10%)
	},
	// State E/F transition to (ev is in error state)
	{
		10000, // state A
		1790,  // state B
		300,   // state C
		165    // state D (150   +10%)
	}
};

// Returns the resistance threshold for the given state that will be transitioned to
uint16_t iec61851_get_cp_resistance_threshold(IEC61851State transition_to_state) {
	return cp_resistance_state[iec61851.state][transition_to_state];
}

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

		if((state == IEC61851_STATE_A ) || (state == IEC61851_STATE_B)) {
			// Turn LED on with timer for standby if we have a state change to state A or B
			led_set_on(false);
		}

		if((iec61851.state != IEC61851_STATE_A) && (state == IEC61851_STATE_A)) {
			// If we change to state A, we know that no car is connected.
			// As long as the contactor is not turned on we can arbitrarily switch phases now.
			iec61851.instant_phase_switch_allowed = true;

			// Reset the "maybe switched under load" flag
			evse.contactor_maybe_switched_under_load = false;

			// If state changed from any non-A state to state A we invalidate the managed current
			// we have to handle the clear on disconnect slots
			charging_slot_handle_disconnect();

			// Start new dc fault test after each charging
			dc_fault.calibration_start = true;
		}

		// If we are in state A or in an error state, we will only allow to turn the contactor on after the next diode check.
		if(state == IEC61851_STATE_A || state == IEC61851_STATE_D || state == IEC61851_STATE_EF) {
			iec61851_diode_error_reset(true);
		}

		if((iec61851.state == IEC61851_STATE_A) && state == IEC61851_STATE_B) {
			iec61851.force_state_f_time = system_timer_get_ms();
		}

		iec61851.state             = state;
		iec61851.last_state_change = system_timer_get_ms();
	}
}

// TODO: We can find out that no cable is connected here
//       if resistance > 10000. Do we want to have a specific
//       state for that?
uint32_t iec61851_get_ma_from_pp_resistance(void) {
	if(adc_result.pp_pe_resistance >= IEC61851_PP_RESISTANCE_13A) {
		return 13000; // 13A
	} else if(adc_result.pp_pe_resistance >= IEC61851_PP_RESISTANCE_20A) {
		return 20000; // 20A
	} else if(adc_result.pp_pe_resistance >= IEC61851_PP_RESISTANCE_32A) {
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
	if(iec61851.force_state_f) {
		return 0;
	}

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
		duty_cycle = ma/60.0f; // For 6A-51A: xA = %duty*0.6
	} else {
		duty_cycle = ma/250.0f + 640; // For 51A-80A: xA= (%duty - 64)*2.5
	}

	// The standard defines 8% as minimum and 100% as maximum
	return BETWEEN(80.0f, duty_cycle, 1000.0f);
}

void iec61851_reset_ev_wakeup(void) {
	iec61851.state_b1b2_transition_time = 0;
	iec61851.state_b1b2_transition_seen = false;
	iec61851.currently_beeing_woken_up = false;
	iec61851.force_state_f = false;

	// If CP is connected we are done with EV wakeup reset
	if(evse_is_cp_connected()) {
		return;
	}

	// If CP is disconnected and phase switch is ongoing we are also done
	if((phase_control.progress_state > 0)  && (phase_control.progress_state < 6)) {
		return;
	}

	// If CP is disconnected by the API we leave it disconnected until it is reconnected by the API
	if(evse.control_pilot_disconnect) {
		return;
	}

	// Otherwise the reset_ev_wakeup was called while a wakeup was ongoing
	// so we have to reconnect here.
	evse_cp_connect();
}

void iec61851_handle_time_in_b2(void) {
	uint32_t ma = iec61851_get_max_ma();
	if(iec61851.state == IEC61851_STATE_B) {
		if(ma != 0) {
			if(iec61851.time_in_b2 == 0) {
				iec61851.time_in_b2 = system_timer_get_ms();
			}
		} else {
			iec61851.time_in_b2 = 0;
		}
	} else {
		iec61851.time_in_b2 = 0;
	}

	if(iec61851.time_in_b2 != 0) {
		const bool use_state_f = (iec61851.force_state_f_time != 0) && system_timer_is_time_elapsed_ms(iec61851.force_state_f_time, 1000*60*60);
		if(system_timer_is_time_elapsed_ms(iec61851.time_in_b2, use_state_f ? 60*1000*5 : 60*1000*3)) {
			evse.car_stopped_charging = true;
		}
	}
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
	// According to the standard we should do the wakeup only once and only for 4 seconds.
	// In the wild we know of EVs that need 30s of wakeup... So we try it two times: First with 4 seconds and after that with 30 seconds.
	if((iec61851.state_b1b2_transition_time != 0) && (!evse.control_pilot_disconnect)) {
		// Only consider to use state F for ev-wakeup if last state C is at least 1 hour ago
		const bool use_state_f = (iec61851.force_state_f_time != 0) && system_timer_is_time_elapsed_ms(iec61851.force_state_f_time, 1000*60*60);

		// "use_state_f" for some reason changes to false while force_state_f is true, we need to set it back to false.
		// Otherwise we might stay in force_state_f forever.
		if(!use_state_f) {
			iec61851.force_state_f = false;
		}

		// Wait for 30 seconds for the EV to wake up for fourth wakeup (state F)
		if(use_state_f && system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000 + 30*1000 + 30*1000 + 4*1000 + 30*1000 + 30*1000)) {
			if(iec61851.force_state_f) {
				iec61851.currently_beeing_woken_up = false;
				iec61851.force_state_f = false;
			} else {
				iec61851.state_b1b2_transition_time = 0;
			}
		}

		// Wait for another 30 seconds for fourth wakeup (state F)
		else if(use_state_f && system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000 + 30*1000 + 30*1000 + 4*1000 + 30*1000)) {
			if(evse.ev_wakeup_enabled) {
				if(!iec61851.force_state_f) {
					iec61851.currently_beeing_woken_up = true;
					iec61851.force_state_f = true;
				}
			}
		}

		// Wait for 30 seconds for the EV to wake up for third wakeup (state F)
		else if(use_state_f && system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000 + 30*1000 + 30*1000 + 4*1000)) {
			if(iec61851.force_state_f) {
				iec61851.currently_beeing_woken_up = false;
				iec61851.force_state_f = false;
			}
		}

		// Wait for another 4 seconds for third wakeup (state F)
		else if(use_state_f && system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000 + 30*1000 + 30*1000)) {
			if(evse.ev_wakeup_enabled) {
				if(!iec61851.force_state_f) {
					iec61851.currently_beeing_woken_up = true;
					iec61851.force_state_f = true;
				}
			}
		}

		// Wait for 30 seconds for the EV to wake up for second wakeup
		else if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000 + 30*1000)) {
			if(!evse_is_cp_connected()) {
				iec61851.currently_beeing_woken_up = false;
				evse_cp_connect();
				if(!use_state_f) {
					iec61851.state_b1b2_transition_time = 0;
				}
			}
		}

		// Wait for another 30 seconds for the second wakeup.
		else if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000 + 30*1000)) {
			if(evse.ev_wakeup_enabled) {
				iec61851.currently_beeing_woken_up = true;
				evse_cp_disconnect();
			}
		}

		// Wait for 4 seconds for the EV to wake up
		else if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000 + 4*1000)) {
			if(!evse_is_cp_connected()) {
				iec61851.currently_beeing_woken_up = false;
				evse_cp_connect();

				// After CP disconnect it is as if the EV was disconnected, so can switch the phases
				// now until the contactor is turned on again.
				iec61851.instant_phase_switch_allowed = true;
			}
		}

		// Wait for 90 seconds for the EV to change resistance and IEC61851 state change to C
		// If this does not happen and ev wakeup is enabled we disconnect CP.
		else if(system_timer_is_time_elapsed_ms(iec61851.state_b1b2_transition_time, 90*1000)) {
			if(evse.ev_wakeup_enabled) {
				iec61851.currently_beeing_woken_up = true;
				evse_cp_disconnect();
			}
		}
	} else {
		// Make sure we can't get stuck in "force_state_f" state
		iec61851.force_state_f = false;
	}
}

void iec61851_state_a(void) {
	// Allow instant phase switch in state A, but only after we bootet and had a chance to check if a
	// car was already connected during the boot.
	if(iec61851.boot_time != 0 && system_timer_is_time_elapsed_ms(iec61851.boot_time, 5000)) {
		iec61851.boot_time = 0;
		iec61851.instant_phase_switch_allowed = true;
	}

	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);

	evse.car_stopped_charging = false;

	iec61851_reset_ev_wakeup();
}

void iec61851_state_b(void) {
	uint32_t ma = iec61851_get_max_ma();

	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	if(hardware_version.is_v4 && iec61851.iso15118_active) {
		evse_set_output(iec61851.iso15118_cp_duty_cycle, false);
	} else {
		evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);
	}

	// If EV wakeup function is enabled, we handle the actual wakeup state machine in state B
	// (when EV is connected but not charging)
	iec61851_handle_ev_wakeup(ma);
}

void iec61851_state_c(void) {
	// Update force_state_f_time. I.e. if we were in state c, we allow wakeup with state F only if some time has passed.
	iec61851.force_state_f_time = system_timer_get_ms();

	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma = iec61851_get_max_ma();
	if(hardware_version.is_v4 && iec61851.iso15118_active) {
		evse_set_output(iec61851.iso15118_cp_duty_cycle, true);
	} else {
		evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
	}

	led_set_breathing();

	evse.car_stopped_charging = false;

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
	if((hardware_version.is_v3 || hardware_version.is_v4) && (contactor_check.error & 1)) { // PE error should have highest priority
		led_set_blinking(4);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if((dc_fault.state & 0b111) != DC_FAULT_NORMAL_CONDITION) {
		led_set_blinking(3);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if(contactor_check.error != 0) {
		led_set_blinking(4);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if((evse.config_jumper_current == EVSE_CONFIG_JUMPER_RESERVED) || (evse.config_jumper_current == EVSE_CONFIG_JUMPER_UNCONFIGURED)) {
		// We don't allow the jumper to be unconfigured
		led_set_blinking(2);
		iec61851_set_state(IEC61851_STATE_EF);
	// For diode error check if
	// * We are in state B or C
	// * We have seen a negative voltage measurement
	// * We see a negative voltage above -10V or a difference of >2V between with and without resistor
	// * The CP contact is connected
	// * We currently apply a PWM (i.e. max ma is not 0)
	// TODO: What do we need to check here when ISO15118 is active?
	} else if(((iec61851.state == IEC61851_STATE_B) || (iec61851.state == IEC61851_STATE_C)) && (!iec61851.iso15118_active) &&
	          (adc[ADC_CHANNEL_VCP1].result_mv[ADC_NEGATIVE_MEASUREMENT] != 0) && (adc[ADC_CHANNEL_VCP2].result_mv[ADC_NEGATIVE_MEASUREMENT] != 0) &&
	          ((adc[ADC_CHANNEL_VCP1].result_mv[ADC_NEGATIVE_MEASUREMENT] > -10000) || (ABS(adc[ADC_CHANNEL_VCP1].result_mv[ADC_NEGATIVE_MEASUREMENT] - adc[ADC_CHANNEL_VCP2].result_mv[ADC_NEGATIVE_MEASUREMENT]) > 2000)) &&
	          (!iec61851.force_state_f && !adc_result.cp_pe_is_ignored && evse_is_cp_connected()) &&
	          (iec61851_get_max_ma() != 0) &&
	          (phase_control.progress_state == 0)) {
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

		// If the CP contact is disconnected or the adc measurement is
		// currently not yielding CP/PE resistance, we stay in the current IEC state.
		// The evse_is_cp_connected function will add an apropriate delay after re-connect.
		if(!iec61851.force_state_f && !adc_result.cp_pe_is_ignored && evse_is_cp_connected()) {
			if(adc_result.cp_pe_resistance > iec61851_get_cp_resistance_threshold(IEC61851_STATE_A)) {
				iec61851_set_state(IEC61851_STATE_A);
			} else if(adc_result.cp_pe_resistance > iec61851_get_cp_resistance_threshold(IEC61851_STATE_B)) {
				iec61851_set_state(IEC61851_STATE_B);
			} else if(adc_result.cp_pe_resistance > iec61851_get_cp_resistance_threshold(IEC61851_STATE_C)) {
				if(charging_slot_get_max_current() == 0) {
					iec61851_set_state(IEC61851_STATE_B);
				} else {
					iec61851_set_state(IEC61851_STATE_C);
				}
			} else if(adc_result.cp_pe_resistance > iec61851_get_cp_resistance_threshold(IEC61851_STATE_D)) {
				led_set_blinking(5);
				iec61851_set_state(IEC61851_STATE_D);
			} else {
				led_set_blinking(5);
				iec61851_set_state(IEC61851_STATE_EF);
			}
		}
	}

	iec61851_handle_time_in_b2();

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
	iec61851.boot_time = system_timer_get_ms();
	if(iec61851.boot_time == 0) {
		iec61851.boot_time = 1;
	}

	iec61851_diode_error_reset(true);
}

