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

#include "xmc_gpio.h"
#include "hardware_version.h"

PhaseControl phase_control;

void phase_control_init(void) {
    memset(&phase_control, 0, sizeof(PhaseControl));

    // default 3-phase
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
        if(phase_control.requested == 1) {
            XMC_GPIO_SetOutputHigh(EVSE_PHASE_SWITCH_PIN);
        } else {
            XMC_GPIO_SetOutputLow(EVSE_PHASE_SWITCH_PIN);
        }
        phase_control.current = phase_control.requested;
    }
}