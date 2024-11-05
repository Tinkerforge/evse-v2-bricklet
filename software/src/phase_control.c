/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * phase_control.c: Control switching between 1- and 3-phase
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

#include "phase_control.h"
#include "configs/config_phase_control.h"
#include "configs/config_evse.h"

#include <string.h>

#include "bricklib2/hal/system_timer/system_timer.h"

#include "xmc_gpio.h"
#include "hardware_version.h"
#include "iec61851.h"
#include "evse.h"
#include "adc.h"
#include "bricklib2/warp/meter.h"

PhaseControl phase_control;

void phase_control_init(void) {
    const bool autoswitch_enabled_save = phase_control.autoswitch_enabled;
    const uint8_t phases_connected_save   = phase_control.phases_connected;
    memset(&phase_control, 0, sizeof(PhaseControl));
    phase_control.phases_connected = phases_connected_save;

    // Default is 3-phase, but we only use 1-phase if only 1 phase is connected
    phase_control.current   = phase_control.phases_connected;
    phase_control.requested = phase_control.phases_connected;

    // No Phase Control and no Autoswitch in EVSE V2
    if(hardware_version.is_v2) {
        phase_control.autoswitch_enabled = false;
        return;
    }

    // In EVSE V3 use auto switch according to persistent value from EEPROM
    phase_control.autoswitch_enabled = autoswitch_enabled_save;

    const XMC_GPIO_CONFIG_t pin_config_output_low = {
        .mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
    };
    const XMC_GPIO_CONFIG_t pin_config_output_high = {
        .mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
    };

    if(phase_control.current == 1) {
        XMC_GPIO_Init(EVSE_PHASE_SWITCH_PIN, &pin_config_output_high);
    } else {
        XMC_GPIO_Init(EVSE_PHASE_SWITCH_PIN, &pin_config_output_low);
    }
}

// Check for auto-switch conditions
// We switch from 3-phase to 1-phase if the car is charging and only using one phase
void phase_control_tick_check_autoswitch(void) {
    // No autoswitch if only 1 phase is connected
    if(phase_control.phases_connected == 1) {
        return;
    }

    if(!phase_control.autoswitch_enabled) {
        phase_control.autoswitch_time = 0;
        phase_control.autoswitch_done = true;
        phase_control.info            = 0;
        return;
    }

    // If a car is charging, it is configured to use 3-phase and a meter is available, we can auto-switch to 1-phase if the car is only using one phase
    if((iec61851.state == IEC61851_STATE_C) && (phase_control.current == 3) && (phase_control.requested == 3) && meter.available && !phase_control.autoswitch_done) {
        // If the car is charging already for at least two seconds
        if(system_timer_is_time_elapsed_ms(iec61851.last_state_change, 2000)) {
            // If the phase 1 uses more than 5A and phase 2 and 3 use less than 0.5A
            if((meter_register_set.current[0].f > 5.0f) && (meter_register_set.current[1].f < 0.5f) && (meter_register_set.current[2].f < 0.5f)) {
                if(phase_control.autoswitch_time == 0) {
                    // If the autos-switch-timer was not yet started we start it
                    phase_control.autoswitch_time = system_timer_get_ms();
                } else if(system_timer_is_time_elapsed_ms(phase_control.autoswitch_time, 15000)) {
                    // All autoswitch conditions have been met for 15 seconds -> switch to 1-phase
                    phase_control.requested       = 1;
                    phase_control.current         = 1;
                    phase_control.autoswitch_time = 0;
                    phase_control.autoswitch_done = true;
                    phase_control.info            = 1;
                    XMC_GPIO_SetOutputHigh(EVSE_PHASE_SWITCH_PIN);
                }
            } else {
                phase_control.autoswitch_time = 0;
                phase_control.info            = 0;
            }
        }
    // If no car is connected and an auto-switch occurred, we go back to the original phase-state
    } else if((iec61851.state == IEC61851_STATE_A) && (phase_control.current == 1) && (phase_control.requested == 1) && phase_control.autoswitch_done) {
        phase_control.requested       = 3;
        phase_control.current         = 3;
        phase_control.autoswitch_time = 0;
        phase_control.autoswitch_done = false;
        phase_control.info            = 0;
        XMC_GPIO_SetOutputLow(EVSE_PHASE_SWITCH_PIN);
    // If the phase state was changed externally and the car is not charging anymore we have to reset autoswitch_done
    } else if((iec61851.state == IEC61851_STATE_A) && phase_control.autoswitch_done) {
        phase_control.autoswitch_time = 0;
        phase_control.autoswitch_done = false;
        phase_control.info            = 0;
    } else {
        phase_control.autoswitch_time = 0;
    }
}

void phase_control_tick(void) {
    // No Phase Control in EVSE V2
    if(hardware_version.is_v2) {
        return;
    }

    phase_control_tick_check_autoswitch();

	// Only switch phase if contactor is not active.
	const bool contactor_inactive = XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN); // Contactor pin is active low
	const bool cp_disconnected = XMC_GPIO_GetInput(EVSE_CP_DISCONNECT_PIN);
	if(contactor_inactive && cp_disconnected) {
		if(phase_control.requested == 1) {
			XMC_GPIO_SetOutputHigh(EVSE_PHASE_SWITCH_PIN);
		} else {
			XMC_GPIO_SetOutputLow(EVSE_PHASE_SWITCH_PIN);
		}
		phase_control.current = phase_control.requested;
	} else if(!phase_control.in_progress) {
		if(phase_control.current != phase_control.requested) {
			// If a different phase setting is requested we set in_progress to true.
			// phase_control_state_phase_change will be called by the IE61851 state machine now.
			phase_control.in_progress = true;
			phase_control.progress_state = 0;
		}
	}
}

void phase_control_done(void) {
    phase_control.progress_state = 0;
    phase_control.progress_state_time = 0;
    phase_control.in_progress = false;
}

// Special phase changing state that is handled similar to the normal IEC61851 states
void phase_control_state_phase_change(void) {
    const bool contactor_active = !XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN);
    const bool cp_connected = !XMC_GPIO_GetInput(EVSE_CP_DISCONNECT_PIN);
    const uint16_t duty_cycle = evse_get_cp_duty_cycle();

    // First call of this functions, lets find out where we are
    if(phase_control.progress_state == 0) {
        // If we happen to be in the middle of the EV wakeup sequence, we stop it here.
        iec61851_reset_ev_wakeup();

        if(duty_cycle != 1000) {
            phase_control.progress_state = 1;
        } else if(contactor_active) {
            phase_control.progress_state = 2;
        } else if(cp_connected) {
            phase_control.progress_state = 3;
        } else {
            phase_control.progress_state = 4;
        }

        phase_control.progress_state_time = system_timer_get_ms();
    }

    switch(phase_control.progress_state) {
        case 1: { // PWM 100%
            evse_set_output(1000, contactor_active);
            phase_control.progress_state = 2;
            phase_control.progress_state_time = system_timer_get_ms();
            break;
        }

        case 2: { // Contactor off
            // According to IEC61851 the car can take 3s to react to PWM change from x% to 100%
            // This is checked by the evse_set_output function, it will delay for 3s if a car is still charging
			evse_set_output(1000, false); // Disable contactor
			if(!contactor_active) {
				phase_control.progress_state = 3;
				phase_control.progress_state_time = system_timer_get_ms();
			}
            break;
        }

        case 3: { // CP disconnect
            if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 100)) {
                // Disconnect CP
                XMC_GPIO_SetOutputHigh(EVSE_CP_DISCONNECT_PIN);
                phase_control.progress_state = 4;
                phase_control.progress_state_time = system_timer_get_ms();
            }
            break;
        }

        case 4: { // Phase switch
            if(phase_control.current == phase_control.requested) {
                phase_control.progress_state = 5;
                phase_control.progress_state_time = system_timer_get_ms();
            }
            break;
        }

        case 5: { // CP Connect
            // Connect CP
            if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 4000)) {
                XMC_GPIO_SetOutputLow(EVSE_CP_DISCONNECT_PIN);
                phase_control.progress_state = 6;
                phase_control.progress_state_time = system_timer_get_ms();
            }
            break;
        }

        case 6: { // PWM x%
            if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 100)) {
                uint32_t ma = iec61851_get_max_ma();
                evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);
                // If the car is currently allowed to charge and the IEC61851 state was C before the state change
                // we wait for the car to start charging again before phase switch is done
                if((ma != 0) && (ma != 1000) && (iec61851.state == IEC61851_STATE_C)) {
                    phase_control.progress_state = 7;
                    phase_control.progress_state_time = system_timer_get_ms();
                } else {
                    phase_control_done();
                }
            }
            break;
        }

        case 7: { // Wait for car to apply 880 Ohm again
            if((adc_result.cp_pe_resistance < IEC61851_CP_RESISTANCE_STATE_B) && (adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_C)) {
                uint32_t ma = iec61851_get_max_ma();
                evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
                phase_control_done();
            // If nobody wants to charge after 3 seconds we give up
            // IEC61851 state will go to B through normal state machine
            } else if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 3000)) {
                evse.charging_time = 0;
                phase_control_done();
            }
        }
    }
}
