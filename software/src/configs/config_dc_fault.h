/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_dc_fault.h: DC fault config
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

#ifndef CONFIG_DC_FAULT_H
#define CONFIG_DC_FAULT_H

#include "xmc_gpio.h"

#define DC_FAULT_X6_PIN_NUM    0
#define DC_FAULT_X30_PIN_NUM   1
#define DC_FAULT_ERR_PIN_NUM   2
#define DC_FAULT_TST_PIN_NUM   3

#define DC_FAULT_X6_PIN        hardware_version_get_port(DC_FAULT_X6_PIN_NUM),  hardware_version_get_pin(DC_FAULT_X6_PIN_NUM)
#define DC_FAULT_X30_PIN       hardware_version_get_port(DC_FAULT_X30_PIN_NUM), hardware_version_get_pin(DC_FAULT_X30_PIN_NUM)
#define DC_FAULT_ERR_PIN       hardware_version_get_port(DC_FAULT_ERR_PIN_NUM), hardware_version_get_pin(DC_FAULT_ERR_PIN_NUM)
#define DC_FAULT_TST_PIN       hardware_version_get_port(DC_FAULT_TST_PIN_NUM), hardware_version_get_pin(DC_FAULT_TST_PIN_NUM)

#define DC_FAULT_V2_X6_PIN     P4_4
#define DC_FAULT_V2_X30_PIN    P4_5
#define DC_FAULT_V2_ERR_PIN    P0_3
#define DC_FAULT_V2_TST_PIN    P0_8

#define DC_FAULT_V3_X6_PIN     P0_1
#define DC_FAULT_V3_X30_PIN    P0_0
#define DC_FAULT_V3_ERR_PIN    P0_3
#define DC_FAULT_V3_TST_PIN    P0_5

#endif