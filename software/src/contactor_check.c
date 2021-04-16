/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * contactor_check.c: Welded/defective contactor check functions
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

#include "contactor_check.h"

#include "configs/config_contactor_check.h"
#include "configs/config_evse.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"

#include <string.h>

#define CONTACTOR_CHECK_INTERVAL 250

ContactorCheck contactor_check;

void contactor_check_init(void) {
	memset(&contactor_check, 0, sizeof(ContactorCheck));

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(CONTACTOR_CHECK_AC1_PIN, &pin_config_input);
	XMC_GPIO_Init(CONTACTOR_CHECK_AC2_PIN, &pin_config_input);

	contactor_check.ac1_last_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC1_PIN);
	contactor_check.ac2_last_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC2_PIN);
}

void contactor_check_tick(void) {
	bool ac1_new_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC1_PIN);
	bool ac2_new_value = XMC_GPIO_GetInput(CONTACTOR_CHECK_AC2_PIN);

	if(ac1_new_value != contactor_check.ac1_last_value) {
		contactor_check.ac1_last_value = ac1_new_value;
		contactor_check.ac1_edge_count++;
	}

	if(ac2_new_value != contactor_check.ac2_last_value) {
		contactor_check.ac2_last_value = ac2_new_value;
		contactor_check.ac2_edge_count++;
	}

	if(system_timer_is_time_elapsed_ms(contactor_check.last_check, CONTACTOR_CHECK_INTERVAL)) {
		contactor_check.last_check = system_timer_get_ms();

		if(contactor_check.invalid_counter > 0) {
			contactor_check.invalid_counter--;
		} else {
			const bool ac1_live = contactor_check.ac1_edge_count > 0;
			const bool ac2_live = contactor_check.ac2_edge_count > 0;

			contactor_check.state = ac1_live | (ac2_live << 1);

			if(!XMC_GPIO_GetInput(EVSE_RELAY_PIN)) {
				// If contact is switched on, we expect to have 230V AC on both sides of it
				switch(contactor_check.state) {
					case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_NLIVE: contactor_check.error = 1; break;
					case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_NLIVE:  contactor_check.error = 2; break;
					case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_LIVE:  contactor_check.error = 3; break;
					case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_LIVE:   contactor_check.error = 0; break;
				}
			} else {
				// If contact is switched off, we expect to have 230V AC on AC1 and nothing on AC2
				switch(contactor_check.state) {
					case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_NLIVE: contactor_check.error = 4; break;
					case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_NLIVE:  contactor_check.error = 0; break;
					case CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_LIVE:  contactor_check.error = 5; break;
					case CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_LIVE:   contactor_check.error = 6; break;
				}
			}
		}

		contactor_check.ac1_edge_count = 0;
		contactor_check.ac2_edge_count = 0;
	}
}