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

#define EVSE_CP_PWM_SLICE_NUMBER       0
#define EVSE_CP_PWM_PIN                P1_0
#define EVSE_CP_DISCONNECT_PIN         P1_4

#define EVSE_MOTOR_ENABLE_SLICE_NUMBER 3 // TODO!
#define EVSE_MOTOR_ENABLE_PIN          P1_5
#define EVSE_MOTOR_PHASE_PIN           P1_6
#define EVSE_MOTOR_FAULT_PIN           P0_1
#define EVSE_MOTOR_INPUT_SWITCH_PIN    P1_1

#define EVSE_RELAY_PIN                 P1_2
#define EVSE_INPUT_GP_PIN              P2_9
#define EVSE_OUTPUT_GP_PIN             P1_3

#define EVSE_CONFIG_JUMPER_PIN0        P0_5
#define EVSE_CONFIG_JUMPER_PIN1        P0_0

#endif