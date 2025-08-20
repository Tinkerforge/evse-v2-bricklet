/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * lock.h: Driver for type 2 socket solenoid lock
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

#ifndef LOCK_H
#define LOCK_H

#include <stdint.h>
#include <stdbool.h>

#define EVSE_LOCK_PWM_PERIOD 4800  // 10kHz

typedef enum {
	LOCK_STATE_INIT,
	LOCK_STATE_OPEN,
	LOCK_STATE_CLOSING,
	LOCK_STATE_CLOSE,
	LOCK_STATE_OPENING,
	LOCK_STATE_ERROR
} LockState;

typedef struct {
	uint32_t closed_time;
	uint32_t opened_time;
	uint32_t last_duty_cycle_update;
	uint16_t duty_cycle;

	LockState state;
} Lock;

extern Lock lock;

LockState lock_get_state(void);
void lock_set_locked(const bool locked);

void lock_init(void);
void lock_tick(void);

#endif