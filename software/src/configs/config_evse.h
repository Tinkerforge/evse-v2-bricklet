/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_evse.h: EVSE 2.0 config
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

#ifndef CONFIG_EVSE_H
#define CONFIG_EVSE_H

#include "xmc_gpio.h"
#include "hardware_version.h"

#define EVSE_CP_PWM_SLICE_NUMBER          0

#define EVSE_CP_PWM_PIN_NUM               4
#define EVSE_CP_DISCONNECT_PIN_NUM        5
#define EVSE_INPUT_GP_PIN_NUM             6
#define EVSE_OUTPUT_GP_PIN_NUM            7
#define EVSE_SHUTDOWN_PIN_NUM             8
#define EVSE_CONFIG_JUMPER_PIN0_NUM       9
#define EVSE_CONFIG_JUMPER_PIN1_NUM       10
#define EVSE_CONTACTOR_PIN_NUM            11
#define EVSE_PHASE_SWITCH_PIN_NUM         12

#define EVSE_CP_PWM_PIN                   hardware_version_get_port(EVSE_CP_PWM_PIN_NUM),         hardware_version_get_pin(EVSE_CP_PWM_PIN_NUM)
#define EVSE_CP_DISCONNECT_PIN            hardware_version_get_port(EVSE_CP_DISCONNECT_PIN_NUM),  hardware_version_get_pin(EVSE_CP_DISCONNECT_PIN_NUM)
#define EVSE_INPUT_GP_PIN                 hardware_version_get_port(EVSE_INPUT_GP_PIN_NUM),       hardware_version_get_pin(EVSE_INPUT_GP_PIN_NUM)
#define EVSE_OUTPUT_GP_PIN                hardware_version_get_port(EVSE_OUTPUT_GP_PIN_NUM),      hardware_version_get_pin(EVSE_OUTPUT_GP_PIN_NUM)
#define EVSE_SHUTDOWN_PIN                 hardware_version_get_port(EVSE_SHUTDOWN_PIN_NUM),       hardware_version_get_pin(EVSE_SHUTDOWN_PIN_NUM)
#define EVSE_CONFIG_JUMPER_PIN0           hardware_version_get_port(EVSE_CONFIG_JUMPER_PIN0_NUM), hardware_version_get_pin(EVSE_CONFIG_JUMPER_PIN0_NUM)
#define EVSE_CONFIG_JUMPER_PIN1           hardware_version_get_port(EVSE_CONFIG_JUMPER_PIN1_NUM), hardware_version_get_pin(EVSE_CONFIG_JUMPER_PIN1_NUM)
#define EVSE_CONTACTOR_PIN                hardware_version_get_port(EVSE_CONTACTOR_PIN_NUM),      hardware_version_get_pin(EVSE_CONTACTOR_PIN_NUM)
#define EVSE_PHASE_SWITCH_PIN             hardware_version_get_port(EVSE_PHASE_SWITCH_PIN_NUM),   hardware_version_get_pin(EVSE_PHASE_SWITCH_PIN_NUM)

#define EVSE_V2_CP_PWM_PIN                P1_0
#define EVSE_V2_CP_DISCONNECT_PIN         P1_4
#define EVSE_V2_INPUT_GP_PIN              P2_9
#define EVSE_V2_OUTPUT_GP_PIN             P1_3
#define EVSE_V2_SHUTDOWN_PIN              P0_9
#define EVSE_V2_CONFIG_JUMPER_PIN0        P0_5
#define EVSE_V2_CONFIG_JUMPER_PIN1        P0_0
#define EVSE_V2_CONTACTOR_PIN             P1_2
#define EVSE_V2_PHASE_SWITCH_PIN          NULL, 0 // Not available

#define EVSE_V3_CP_PWM_PIN                P1_4
#define EVSE_V3_CP_DISCONNECT_PIN         P4_4
#define EVSE_V3_INPUT_GP_PIN              NULL, 0 // Not available
#define EVSE_V3_OUTPUT_GP_PIN             NULL, 0 // Not available
#define EVSE_V3_SHUTDOWN_PIN              P4_6
#define EVSE_V3_CONFIG_JUMPER_PIN0        P4_5
#define EVSE_V3_CONFIG_JUMPER_PIN1        P2_9
#define EVSE_V3_CONTACTOR_PIN             P1_6
#define EVSE_V3_PHASE_SWITCH_PIN          P1_5

#endif
