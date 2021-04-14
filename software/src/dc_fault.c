/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * dc_fault.c: DC fault detection
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

#include "dc_fault.h"

#include "configs/config_dc_fault.h"

#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#include "xmc_gpio.h"

void dc_fault_init(void) {
	memset(&dc_fault, 0, sizeof(DCFault));

	const XMC_GPIO_CONFIG_t pin_config_output_low = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	const XMC_GPIO_CONFIG_t pin_config_output_high = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(DC_FAULT_TST_PIN, &pin_config_output_high);
	XMC_GPIO_Init(DC_FAULT_X6_PIN,  &pin_config_input);
	XMC_GPIO_Init(DC_FAULT_X30_PIN, &pin_config_input);
	XMC_GPIO_Init(DC_FAULT_ERR_PIN, &pin_config_input);
}

uint32_t tmp_time = 0;
void dc_fault_tick(void) {
	dc_fault.x6    = XMC_GPIO_GetInput(DC_FAULT_X6_PIN);
	dc_fault.x30   = XMC_GPIO_GetInput(DC_FAULT_X30_PIN);
	dc_fault.error = XMC_GPIO_GetInput(DC_FAULT_ERR_PIN);

	if(!dc_fault.x6 && !dc_fault.x30 && !dc_fault.error) {
		dc_fault.state = DC_FAULT_NORMAL_CONDITION;
	} else if(dc_fault.x6 && dc_fault.x30 && !dc_fault.error) {
		dc_fault.state = DC_FAULT_6MA;
	} else if(dc_fault.x6 && dc_fault.x30 && dc_fault.error) {
		dc_fault.state = DC_FAULT_SYSTEM;
	} else {
		dc_fault.state = DC_FAULT_UNKOWN;
	}

	if(dc_fault.state != DC_FAULT_NORMAL_CONDITION) {
		dc_fault.last_fault_time = system_timer_get_ms();
	}

	if(system_timer_is_time_elapsed_ms(tmp_time, 250)) {
		tmp_time = system_timer_get_ms();
//		logd("DC GPIO X6: %d, X30: %d, ERR: %d\n\r", XMC_GPIO_GetInput(DC_FAULT_X6_PIN), XMC_GPIO_GetInput(DC_FAULT_X30_PIN), XMC_GPIO_GetInput(DC_FAULT_ERR_PIN));
	}
}