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

PhaseControl phase_control;

void phase_control_init(void) {
    memset(&phase_control, 0, sizeof(PhaseControl));

    // Default 3-phase
    phase_control.current = 3; 
    phase_control.requested = 3;

    // No Phase Control in EVSE V2
    if(hardware_version.is_v2) {
        return;
    }

    const XMC_GPIO_CONFIG_t pin_config_output_low = {
        .mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
        .output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
    };
    XMC_GPIO_Init(EVSE_PHASE_SWITCH_PIN, &pin_config_output_low);
}

void phase_control_tick(void) {
    // No Phase Control in EVSE V2
    if(hardware_version.is_v2) {
        return;
    }

    if(phase_control.current != phase_control.requested) {
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
            // We wait up to 3.1s after PWM is set to 100% before we turn contactor off
            if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 3100) || (adc_result.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_B)) {
                // Disable contactor
                evse_set_output(1000, false);
                if(!contactor_active) {
                    phase_control.progress_state = 3;
                    phase_control.progress_state_time = system_timer_get_ms();
                }
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
            if(system_timer_is_time_elapsed_ms(phase_control.progress_state_time, 2000)) {
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
                phase_control_done();
            }
        }
    }
}