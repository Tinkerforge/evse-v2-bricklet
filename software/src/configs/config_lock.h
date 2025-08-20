/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_lock.h: EVSE Lock config (hardware version 4 only)
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

#ifndef CONFIG_LOCK_H
#define CONFIG_LOCK_H

#include "xmc_gpio.h"
#include "xmc_ccu4.h"

#define EVSE_LOCK_IN1_TRANSFER     (XMC_CCU4_SHADOW_TRANSFER_SLICE_1 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_1)
#define EVSE_LOCK_IN2_TRANSFER     (XMC_CCU4_SHADOW_TRANSFER_SLICE_2 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_2)
#define EVSE_LOCK_IN1_SLICE        CCU40_CC41
#define EVSE_LOCK_IN2_SLICE        CCU40_CC42
#define EVSE_LOCK_IN1_PWM_MODE     XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT2
#define EVSE_LOCK_IN2_PWM_MODE     XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT4
#define EVSE_LOCK_IN1_SLICE_NUMBER 1
#define EVSE_LOCK_IN2_SLICE_NUMBER 2

#define EVSE_LOCK_IN1_PIN          P1_1
#define EVSE_LOCK_IN2_PIN          P0_2
#define EVSE_LOCK_FEEDBACK_PIN     P0_9

#endif