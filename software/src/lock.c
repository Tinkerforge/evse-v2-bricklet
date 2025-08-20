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
#include "configs/config_lock.h"
#include "evse.h"
#include "hardware_version.h"

Lock lock;

void lock_pwm_set(const uint16_t in1_value, const uint16_t in2_value) {
	if((in1_value > EVSE_LOCK_PWM_PERIOD) || (in2_value > EVSE_LOCK_PWM_PERIOD)) {
		return;
	}

	XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LOCK_IN1_SLICE, EVSE_LOCK_PWM_PERIOD-in1_value);
	XMC_CCU4_EnableShadowTransfer(CCU40, EVSE_LOCK_IN1_TRANSFER);
	XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LOCK_IN2_SLICE, EVSE_LOCK_PWM_PERIOD-in2_value);
	XMC_CCU4_EnableShadowTransfer(CCU40, EVSE_LOCK_IN2_TRANSFER);
}

void lock_pwm_init(void) {
	const XMC_CCU4_SLICE_COMPARE_CONFIG_t compare_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = 0,
		.prescaler_initval   = XMC_CCU4_SLICE_PRESCALER_2,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = 0
	};

	const XMC_GPIO_CONFIG_t gpio_out_config_in1	= {
		.mode                = EVSE_LOCK_IN1_PWM_MODE,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};
	const XMC_GPIO_CONFIG_t gpio_out_config_in2	= {
		.mode                = EVSE_LOCK_IN2_PWM_MODE,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

	XMC_CCU4_Init(CCU40, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU4_StartPrescaler(CCU40);
	XMC_CCU4_SLICE_CompareInit(EVSE_LOCK_IN1_SLICE, &compare_config);
	XMC_CCU4_SLICE_CompareInit(EVSE_LOCK_IN2_SLICE, &compare_config);

	// Set the period and compare register values
	XMC_CCU4_SLICE_SetTimerPeriodMatch(EVSE_LOCK_IN1_SLICE, EVSE_LOCK_PWM_PERIOD-1);
	XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LOCK_IN1_SLICE, EVSE_LOCK_PWM_PERIOD);
	XMC_CCU4_EnableShadowTransfer(CCU40, EVSE_LOCK_IN1_TRANSFER);

	XMC_CCU4_SLICE_SetTimerPeriodMatch(EVSE_LOCK_IN2_SLICE, EVSE_LOCK_PWM_PERIOD-1);
	XMC_CCU4_EnableShadowTransfer(CCU40, EVSE_LOCK_IN2_TRANSFER);
	XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LOCK_IN2_SLICE, EVSE_LOCK_PWM_PERIOD);

	XMC_GPIO_Init(EVSE_LOCK_IN1_PIN, &gpio_out_config_in1);
	XMC_GPIO_Init(EVSE_LOCK_IN2_PIN, &gpio_out_config_in2);

	XMC_CCU4_EnableClock(CCU40, EVSE_LOCK_IN1_SLICE_NUMBER);
	XMC_CCU4_EnableClock(CCU40, EVSE_LOCK_IN2_SLICE_NUMBER);
	XMC_CCU4_SLICE_StartTimer(EVSE_LOCK_IN1_SLICE);
	XMC_CCU4_SLICE_StartTimer(EVSE_LOCK_IN2_SLICE);
}

LockState lock_get_state(void) {
	return lock.state;
}

void lock_set_locked(const bool locked) {
	if(locked) {
		if((lock.state == LOCK_STATE_CLOSING) || (lock.state == LOCK_STATE_CLOSE)) {
			return;
		}

		lock.state = LOCK_STATE_CLOSING;
		lock.duty_cycle = 0;
		lock_pwm_set(0, 0);
	} else {
		if((lock.state == LOCK_STATE_OPENING) || (lock.state == LOCK_STATE_OPEN)) {
			return;
		}

		lock.state = LOCK_STATE_OPENING;
		lock.duty_cycle = 0;
		lock_pwm_set(0, 0);
	}
}

void lock_init(void) {
	if(!hardware_version.is_v4) {
		return;
	}

	memset(&lock, 0, sizeof(Lock));

	lock_pwm_init();

	const XMC_GPIO_CONFIG_t pin_config_feedback = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_LOCK_FEEDBACK_PIN, &pin_config_feedback);
}

void lock_tick(void) {
	if(!hardware_version.is_v4) {
		return;
	}

	if((lock.state == LOCK_STATE_CLOSING) || (lock.state == LOCK_STATE_OPENING)) {
		// This creates about 1 second of slow PWM run-up to reduce power draw from lock motor
		if(system_timer_is_time_elapsed_ms(lock.last_duty_cycle_update, 1)) {
			lock.last_duty_cycle_update = system_timer_get_ms();
			lock.duty_cycle += 3;
			if(lock.duty_cycle > EVSE_LOCK_PWM_PERIOD) {
				lock.duty_cycle = EVSE_LOCK_PWM_PERIOD;
			}
		}

		const bool lock_closed = XMC_GPIO_GetInput(EVSE_LOCK_FEEDBACK_PIN);
		const uint32_t lock_time = system_timer_get_ms();

		if(lock_closed) {
			lock.opened_time = 0;
			if(lock.closed_time == 0) {
				lock.closed_time = lock_time;
			}
		} else {
			lock.closed_time = 0;
			if(lock.opened_time == 0) {
				lock.opened_time = lock_time;
			}
		}

		if((lock.state == LOCK_STATE_CLOSING) && lock_closed && system_timer_is_time_elapsed_ms(lock.closed_time, 75)) {
			lock.state = LOCK_STATE_CLOSE;
			lock.duty_cycle = 0;
			lock_pwm_set(0, 0);
			return;
		} else if((lock.state == LOCK_STATE_OPENING) && !lock_closed && system_timer_is_time_elapsed_ms(lock.opened_time, 50)) {
			lock.state = LOCK_STATE_OPEN;
			lock.duty_cycle = 0;
			lock_pwm_set(0, 0);
			return;
		}

		if(lock.state == LOCK_STATE_CLOSING) {
			lock_pwm_set(lock.duty_cycle, 0);
		} else {
			lock_pwm_set(0, lock.duty_cycle);
		}
	} else {
		lock.duty_cycle = 0;
		lock_pwm_set(0, 0);
	}
}