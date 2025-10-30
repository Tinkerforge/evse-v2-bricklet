/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * phase_control.h: Control switching between 1- and 3-phase
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

#ifndef PHASE_CONTROL_H
#define PHASE_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

#define PHASE_CONTROL_CP_RECONNECT_TIME_DEFAULT    (45*1000UL) // ms
#define PHASE_CONTROL_CP_RECONNECT_TIME_INCREMENT   (5*1000UL) // ms
#define PHASE_CONTROL_CP_RECONNECT_TIME_MINIMUM    (10*1000UL) // ms
#define PHASE_CONTROL_CP_RECONNECT_TIME_UNDER_LOAD (60*1000UL) // ms

typedef struct {
	uint8_t current;
	uint8_t requested;

	uint8_t info; // 0 = normal, 1 = 1-phase forced by auto-switch

	bool in_progress;
	uint8_t progress_state;
	uint32_t progress_state_time;

	uint32_t autoswitch_time;
	bool autoswitch_done;

	bool autoswitch_enabled;
	uint8_t phases_connected;
	uint8_t cp_reconnect_time;

	uint32_t meter_phase_check_time;
} PhaseControl;

extern PhaseControl phase_control;

void phase_control_init(void);
void phase_control_tick(void);
void phase_control_state_phase_change(void);

#endif
