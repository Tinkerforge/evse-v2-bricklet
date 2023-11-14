/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_tmp1074n.h: Config for TMP1074N temperature sensor 
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

#ifndef CONFIG_TMP1075N_H
#define CONFIG_TMP1075N_H

#include "xmc_gpio.h"
#include "xmc_i2c.h"

#define TMP1075N_I2C_BAUDRATE         100000

#define TMP1075N_I2C_ADDRESS          0b1001001
#define TMP1075N_I2C                  XMC_I2C0_CH1

#define TMP1075N_SCL_PORT             XMC_GPIO_PORT0
#define TMP1075N_SCL_PIN              8
#define TMP1075N_SCL_PIN_MODE         XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT7
#define TMP1075N_SCL_INPUT            XMC_USIC_CH_INPUT_DX1
#define TMP1075N_SCL_SOURCE           1
#define TMP1075N_SCL_FIFO_SIZE        XMC_USIC_CH_FIFO_SIZE_16WORDS
#define TMP1075N_SCL_FIFO_POINTER     32

#define TMP1075N_SDA_PORT             XMC_GPIO_PORT0
#define TMP1075N_SDA_PIN              7
#define TMP1075N_SDA_PIN_MODE         XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN_ALT7
#define TMP1075N_SDA_INPUT            XMC_USIC_CH_INPUT_DX0
#define TMP1075N_SDA_SOURCE           2
#define TMP1075N_SDA_FIFO_SIZE        XMC_USIC_CH_FIFO_SIZE_16WORDS
#define TMP1075N_SDA_FIFO_POINTER     48

#endif
