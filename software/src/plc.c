/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * plc.c: EVSE 2.0 PLC driver
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

#include "plc.h"
#include "configs/config_evse.h"

#include "xmc_gpio.h"

#include "hardware_version.h"

#include <string.h>

PLC plc;

void plc_init(void) {
    if(hardware_version.is_v4) {
        memset(&plc, 0, sizeof(PLC));
        plc.enable = true;

        const XMC_GPIO_CONFIG_t pin_config_output = {
            .mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
            .output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
        };

        if(hardware_version.is_v4) {
            XMC_GPIO_Init(EVSE_V4_PLC_RESET_PIN, &pin_config_output);
        }
    }
}

void plc_tick(void) {
    if(hardware_version.is_v4) {
        if(plc.enable) {
            XMC_GPIO_SetOutputHigh(EVSE_V4_PLC_RESET_PIN);
        } else {
            XMC_GPIO_SetOutputLow(EVSE_V4_PLC_RESET_PIN);
        }
    }
}
