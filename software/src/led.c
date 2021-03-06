/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led.c: EVSE 2.0 LED driver
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

#include "led.h"
#include "configs/config_led.h"

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"

#include "xmc_ccu4.h"

#include <string.h>

LED led;


// CIE1931 correction table
const uint16_t led_cie1931[256] = {
	0, 28, 57, 85, 114, 142, 171, 199, 228, 256, 
	285, 313, 342, 370, 399, 427, 456, 484, 513, 541, 
	570, 598, 627, 658, 689, 721, 755, 789, 825, 861, 
	899, 937, 977, 1018, 1060, 1103, 1147, 1192, 1239, 1287, 
	1336, 1386, 1437, 1490, 1544, 1599, 1656, 1714, 1773, 1834, 
	1896, 1959, 2024, 2090, 2157, 2226, 2297, 2369, 2442, 2517, 
	2593, 2671, 2751, 2832, 2914, 2999, 3085, 3172, 3261, 3352, 
	3444, 3538, 3634, 3732, 3831, 3932, 4035, 4139, 4245, 4354, 
	4464, 4575, 4689, 4804, 4922, 5041, 5162, 5285, 5410, 5537, 
	5666, 5797, 5930, 6065, 6202, 6341, 6482, 6626, 6771, 6918, 
	7068, 7220, 7373, 7529, 7687, 7848, 8010, 8175, 8342, 8512, 
	8683, 8857, 9033, 9212, 9393, 9576, 9762, 9949, 10140, 10333, 
	10528, 10725, 10926, 11128, 11333, 11541, 11751, 11963, 12179, 12396, 
	12617, 12840, 13065, 13293, 13524, 13757, 13993, 14232, 14474, 14718, 
	14965, 15215, 15467, 15722, 15980, 16241, 16505, 16771, 17041, 17313, 
	17588, 17866, 18147, 18431, 18717, 19007, 19300, 19596, 19894, 20196, 
	20501, 20809, 21119, 21433, 21750, 22071, 22394, 22720, 23050, 23383, 
	23719, 24058, 24400, 24746, 25095, 25447, 25802, 26161, 26523, 26888, 
	27257, 27629, 28004, 28383, 28765, 29151, 29540, 29932, 30328, 30728, 
	31131, 31537, 31947, 32360, 32777, 33198, 33622, 34050, 34481, 34916, 
	35355, 35797, 36243, 36693, 37146, 37603, 38064, 38529, 38997, 39469, 
	39945, 40425, 40908, 41396, 41887, 42382, 42881, 43384, 43891, 44401, 
	44916, 45435, 45957, 46484, 47015, 47549, 48088, 48631, 49178, 49728, 
	50283, 50843, 51406, 51973, 52545, 53120, 53700, 54284, 54873, 55465, 
	56062, 56663, 57269, 57878, 58492, 59111, 59733, 60360, 60992, 61627, 
	62268, 62912, 63561, 64215, 64873, 65535, 
};

#define LED_MAX_DUTY_CYCLE 6553
#define LED_MIN_DUTY_CYLCE 0

#define LED_ON  LED_MIN_DUTY_CYLCE
#define LED_OFF LED_MAX_DUTY_CYCLE

void led_set_duty_cycle(const uint16_t compare_value) {
	XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LED_SLICE, compare_value);
    XMC_CCU4_EnableShadowTransfer(EVSE_LED_CCU, (XMC_CCU4_SHADOW_TRANSFER_SLICE_0 << (EVSE_LED_SLICE_NUMBER*4)) |
    		                                    (XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 << (EVSE_LED_SLICE_NUMBER*4)));
}

void led_set_blinking(const uint8_t num) {
	// Check if we are already blinking with the correct blink amount
	if((led.state == LED_STATE_BLINKING) && (led.blink_num == num)) {
		return;
	}

	led.state           = LED_STATE_BLINKING;
	led.blink_num       = num;
	led.blink_count     = num;
	led.blink_on        = false;
	led.blink_last_time = system_timer_get_ms();

	led_set_duty_cycle(LED_OFF);
}

// Called whenever there is activity
// LED will go to standby after 15 minutes again
void led_set_on(void) {
	led.on_time = system_timer_get_ms();
	led.state = LED_STATE_ON;
	led_set_duty_cycle(LED_ON);
}

void led_init(void) {
	memset(&led, 0, sizeof(LED));

	const XMC_CCU4_SLICE_COMPARE_CONFIG_t compare_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = 0,
		.prescaler_initval   = 0,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = 0
	};

	const XMC_GPIO_CONFIG_t gpio_out_config	= {
		.mode                = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

    XMC_CCU4_Init(EVSE_LED_CCU, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(EVSE_LED_CCU);
    XMC_CCU4_SLICE_CompareInit(EVSE_LED_SLICE, &compare_config);

    // Set the period and compare register values
    XMC_CCU4_SLICE_SetTimerPeriodMatch(EVSE_LED_SLICE, LED_MAX_DUTY_CYCLE-1);
    XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_LED_SLICE, 0);

    XMC_CCU4_EnableShadowTransfer(EVSE_LED_CCU, (XMC_CCU4_SHADOW_TRANSFER_SLICE_0 << (EVSE_LED_SLICE_NUMBER*4)) |
    		                             (XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 << (EVSE_LED_SLICE_NUMBER*4)));

    XMC_GPIO_Init(EVSE_LED_PIN, &gpio_out_config);

    XMC_CCU4_EnableClock(EVSE_LED_CCU, EVSE_LED_SLICE_NUMBER);
    XMC_CCU4_SLICE_StartTimer(EVSE_LED_SLICE);

	led_set_duty_cycle(LED_OFF);

	led.state = LED_STATE_FLICKER;
}

void led_tick_status_off(void) {
	led_set_duty_cycle(LED_OFF);
}

void led_tick_status_on(void) {
	if(system_timer_is_time_elapsed_ms(led.on_time, LED_STANDBY_TIME)) {
		led.state = LED_STATE_OFF;
		led_set_duty_cycle(LED_OFF);
	} else {
		led_set_duty_cycle(LED_ON);
	}
}

void led_tick_status_blinking(void) {
	if(led.blink_count >= led.blink_num) {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_WAIT)) {
			led.blink_last_time = system_timer_get_ms();
			led.blink_count = 0;
		}
	} else if(led.blink_on) {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_ON)) {
			led.blink_last_time = system_timer_get_ms();
			led_set_duty_cycle(LED_OFF);
			led.blink_on = false;
			led.blink_count++;
		}
	} else {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_OFF)) {
			led.blink_last_time = system_timer_get_ms();
			led_set_duty_cycle(LED_ON);
			led.blink_on = true;
		}
	}
}

void led_tick_status_flicker(void) {
	if(system_timer_is_time_elapsed_ms(led.flicker_last_time, LED_FLICKER_DURATION)) {
		led_set_duty_cycle(led.flicker_on ? LED_ON : LED_OFF);
		led.flicker_last_time = system_timer_get_ms();
		led.flicker_on        = ! led.flicker_on;
	}
}

void led_tick_status_breathing(void) {
	static uint32_t last_breath_time = 0;
	static int16_t last_breath_index = 0;
	static bool up = true;

	if(!system_timer_is_time_elapsed_ms(last_breath_time, 5)) {
		return;
	}
	last_breath_time = system_timer_get_ms();

	if(up) {
		last_breath_index += 1;
	} else {
		last_breath_index -= 1;
	}
	last_breath_index = BETWEEN(0, last_breath_index, 255);
	
	if(last_breath_index == 0) {
		up = true;
	} else if(last_breath_index == 255) {
		up = false;
	}

	led_set_duty_cycle(6553 - led_cie1931[last_breath_index]/10);
}

void led_tick(void) {
	switch(led.state) {
		case LED_STATE_OFF:       led_tick_status_off();       break;
		case LED_STATE_ON:        led_tick_status_on();        break;
		case LED_STATE_BLINKING:  led_tick_status_blinking();  break;
		case LED_STATE_FLICKER:   led_tick_status_flicker();   break;
		case LED_STATE_BREATHING: led_tick_status_breathing(); break;
	}
}