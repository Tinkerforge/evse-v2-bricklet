/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * frequency.h: Mains frequency measurement from PE check 50 Hz signal
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

#ifndef FREQUENCY_H
#define FREQUENCY_H

#include <stdint.h>
#include <stdbool.h>

#include "configs/config_frequency.h"

#define FREQUENCY_BUFFER_SIZE 256
#define FREQUENCY_TIMEOUT_MS  1000

// Sanity bounds for a single period measurement (reject glitches / harmonics)
#define FREQUENCY_MIN_TICKS   20000
#define FREQUENCY_MAX_TICKS   50000

typedef struct {
	uint16_t period_buffer[FREQUENCY_BUFFER_SIZE];
	uint8_t buffer_index;
	uint16_t last_timer_value;

	uint32_t frequency; // Measured mains frequency in 1/1000 Hz (e.g. 50000 = 50.00 Hz)
	bool valid; // True once enough samples have been collected and signal is present

	uint32_t last_edge_ms;
	uint32_t last_update;
	uint32_t start_ms;
} Frequency;

extern Frequency frequency;

void frequency_init(void);
void frequency_tick(void);

#endif
