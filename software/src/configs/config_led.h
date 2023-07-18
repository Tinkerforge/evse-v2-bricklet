/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_led.h: EVSE LED config
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

#ifndef CONFIG_LED_H
#define CONFIG_LED_H

#include "xmc_gpio.h"

// EVSE V2 -> only one blue LED
// CCU4_1.OUT_2
#define EVSE_V2_LED_CCU             CCU41
#define EVSE_V2_LED_SLICE           CCU41_CC42
#define EVSE_V2_LED_SLICE_NUMBER    2 
#define EVSE_V2_LED_PIN             P4_6

// EVSE V3 -> RGB LED
#define EVSE_V3_LED_CCU           CCU80

#define EVSE_V3_LED_R_SLICE         CCU80_CC80
#define EVSE_V3_LED_R_SLICE_NUMBER  0
#define EVSE_V3_LED_R_PIN           P1_0

#define EVSE_V3_LED_G_SLICE         CCU80_CC83
#define EVSE_V3_LED_G_SLICE_NUMBER  3
#define EVSE_V3_LED_G_PIN           P1_3

#define EVSE_V3_LED_B_SLICE         CCU80_CC82
#define EVSE_V3_LED_B_SLICE_NUMBER  2 
#define EVSE_V3_LED_B_PIN           P1_2

#endif