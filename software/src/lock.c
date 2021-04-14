/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * lock.c: Driver for type 2 socket solenoid lock
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

#include "lock.h"

#include <string.h>
#include <stdint.h>

#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "configs/config_evse.h"
#include "evse.h"

Lock lock;

LockState lock_get_state(void) {
	return lock.state;
}

void lock_set_locked(const bool locked) {
	if(locked) {
		if(lock.state == LOCK_STATE_CLOSING) {
			return;
		}

		lock.state = LOCK_STATE_CLOSING;
		XMC_GPIO_SetOutputHigh(EVSE_MOTOR_PHASE_PIN);
	} else {
		if(lock.state == LOCK_STATE_OPENING) {
			return;
		}

		lock.state = LOCK_STATE_OPENING;
		XMC_GPIO_SetOutputLow(EVSE_MOTOR_PHASE_PIN);
	}

	lock.duty_cycle = 3200;
	ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, lock.duty_cycle);
}

void lock_init(void) {
	memset(&lock, 0, sizeof(Lock));
}

void lock_tick(void) {
	if((lock.state == LOCK_STATE_CLOSING) || (lock.state == LOCK_STATE_OPENING)) {
		if(lock.state == LOCK_STATE_CLOSING) {
			XMC_GPIO_SetOutputHigh(EVSE_MOTOR_PHASE_PIN);
		} else {
			XMC_GPIO_SetOutputLow(EVSE_MOTOR_PHASE_PIN);
		}

		// This creates about 1 second of slow PWM run-up to reduce power draw from lock motor
		if(system_timer_is_time_elapsed_ms(lock.last_duty_cycle_update, 1)) {
			lock.last_duty_cycle_update = system_timer_get_ms();
			if(lock.duty_cycle >= 0) {
				lock.duty_cycle -= 5;
			}
		}

		if(evse.has_lock_switch) {
			if(!XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN)) {
				if(lock.state == LOCK_STATE_CLOSING) {
					if(lock.last_input_switch_seen == 0) {
						lock.last_input_switch_seen = system_timer_get_ms();
					}

					if((lock.last_input_switch_seen != 0) && system_timer_is_time_elapsed_ms(lock.last_input_switch_seen, 500)) {
						lock.state = LOCK_STATE_CLOSE;
						lock.last_input_switch_seen = 0;
						lock.lock_start = 0;
						lock.duty_cycle = 6400;
					}
				}
			} else {
				if(lock.state == LOCK_STATE_OPENING) {
					if(lock.last_input_switch_seen == 0) {
						lock.last_input_switch_seen = system_timer_get_ms();
					}

					if((lock.last_input_switch_seen != 0) && system_timer_is_time_elapsed_ms(lock.last_input_switch_seen, 500)) {
						lock.state = LOCK_STATE_OPEN;
						lock.last_input_switch_seen = 0;
						lock.lock_start = 0;
						lock.duty_cycle = 6400;
					}
				}
			}
		} else {
			if(lock.lock_start == 0) {
				lock.lock_start = system_timer_get_ms();
			}
			if(lock.state == LOCK_STATE_CLOSING) {
				if(system_timer_is_time_elapsed_ms(lock.lock_start, 2000)) {
					lock.state = LOCK_STATE_CLOSE;
					lock.lock_start = 0;
					lock.last_input_switch_seen = 0;
					lock.duty_cycle = 6400;
				}
			} else if(lock.state == LOCK_STATE_OPENING) {
				if(system_timer_is_time_elapsed_ms(lock.lock_start, 2000)) {
					lock.state = LOCK_STATE_OPEN;
					lock.lock_start = 0;
					lock.last_input_switch_seen = 0;
					lock.duty_cycle = 6400;
				}
			}
		}
		
		ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, lock.duty_cycle);
	} else {
		lock.duty_cycle = 6400;
		ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, lock.duty_cycle);
	}
}