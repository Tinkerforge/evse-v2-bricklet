/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * button.h: EVSE button driver
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

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	BUTTON_STATE_RELEASED,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASED_DEBOUNCE,
	BUTTON_STATE_PRESSED_DEBOUNCE
} ButtonState;

typedef struct {
	ButtonState state;

	bool last_value;
	uint32_t last_change_time;
	uint32_t debounce_time;

	uint8_t configuration;

	uint32_t press_time;
	uint32_t release_time;

	uint32_t boot_press_time;
	uint32_t boot_press_start;
	bool boot_done;
} Button;

extern Button button;

void button_init(void);
void button_tick(void);

#endif
