/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_frequency.h: Config for mains frequency measurement via PE check signal
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

#ifndef CONFIG_FREQUENCY_H
#define CONFIG_FREQUENCY_H

#include "xmc_ccu4.h"
#include "xmc_eru.h"
#include "xmc1_eru_map.h"

#define FREQUENCY_TIMER_MODULE    CCU40
#define FREQUENCY_TIMER_SLICE     CCU40_CC43
#define FREQUENCY_TIMER_SLICE_NUM 3
#define FREQUENCY_TIMER_PRESCALER XMC_CCU4_SLICE_PRESCALER_64
#define FREQUENCY_TIMER_CLOCK_HZ  1500000UL
#define FREQUENCY_MILLIHZ_FACTOR  (FREQUENCY_TIMER_CLOCK_HZ * 1000UL)

// ERU: P2_8 -> ERU0 ETL3 Input B1
#define FREQUENCY_ERU             XMC_ERU0
#define FREQUENCY_ERU_ETL_CHANNEL 3
#define FREQUENCY_ERU_OGU_CHANNEL 3
#define FREQUENCY_ERU_INPUT_B     ERU0_ETL3_INPUTB_P2_8

// IRQ: ERU0_SR3 -> IRQ6
#define FREQUENCY_IRQ_NUM         6
#define FREQUENCY_IRQCTRL         XMC_SCU_IRQCTRL_ERU0_SR3_IRQ6


#endif
