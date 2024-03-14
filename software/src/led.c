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
#include "xmc_ccu8.h"

#include "hardware_version.h"

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

#define LED_MAX_DUTY_CYCLE 0xFFFF
#define LED_MIN_DUTY_CYCLE 0

#define LED_ON  LED_MAX_DUTY_CYCLE
#define LED_OFF LED_MIN_DUTY_CYCLE


void led_hsv_to_rgb(const uint16_t h, const uint8_t s, const uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    if(s == 0) {
        *r = v;
		*g = v;
		*b = v;
    } else {
        const uint8_t i = h / 60;
        const uint8_t p = (256*v - s*v) / 256;

        if(i & 1) {
            const int32_t q = (256*60*v - h*s*v + 60*s*v*i) / (256*60);
            switch(i) {
                case 1: *r = q; *g = v; *b = p; break;
                case 3: *r = p; *g = q; *b = v; break;
                case 5: *r = v; *g = p; *b = q; break;
            }
        } else {
            const int32_t t = (256*60*v + h*s*v - 60*s*v*(i+1)) / (256*60);
            switch(i) {
                case 0: *r = v; *g = t; *b = p; break;
                case 2: *r = p; *g = v; *b = t; break;
                case 4: *r = t; *g = p; *b = v; break;
            }
        }
    }
}

void led_update(const uint16_t h, const uint8_t s, const uint8_t v) {
	led.h = h;
	led.s = s;
	led.v = v;
	led_hsv_to_rgb(h, s, v, &led.r, &led.g, &led.b);

	uint16_t compare_value_b = led_cie1931[led.b];
	if(hardware_version.is_v2) {
		XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_V2_LED_SLICE, compare_value_b);
    	XMC_CCU4_EnableShadowTransfer(EVSE_V2_LED_CCU, (XMC_CCU4_SHADOW_TRANSFER_SLICE_0 << (EVSE_V2_LED_SLICE_NUMBER*4)) | (XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 << (EVSE_V2_LED_SLICE_NUMBER*4)));
	} else if(hardware_version.is_v3) {
		uint16_t compare_value_r = led_cie1931[led.r];
		uint16_t compare_value_g = led_cie1931[led.g];
		XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_R_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, compare_value_r);
		XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_G_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, compare_value_g);
		XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_B_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, compare_value_b);
		XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_R_CCU, EVSE_V3_LED_R_SHADOW);
		XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_G_CCU, EVSE_V3_LED_G_SHADOW);
		XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_B_CCU, EVSE_V3_LED_B_SHADOW);
	}
}

void led_reset_api_state(void) {
	led.api_indication   = -1;
	led.api_start        = 0;
	led.api_duration     = 0;

	led.api_ack_counter  = 0;
	led.api_ack_index    = 0;
	led.api_ack_time     = 0;

	led.api_nack_counter = 0;
	led.api_nack_index   = 0;
	led.api_nack_time    = 0;

	led.api_nag_counter  = 0;
	led.api_nag_index    = 0;
	led.api_nag_time     = 0;
}

void led_set_breathing(void) {
	// Check if we are already breathing state
	if(led.state == LED_STATE_BREATHING) {
		return;
	}

	// API has higher priority than breathing
	if(led.state == LED_STATE_API) {
		return;
	}

	// Otherwise start breathing from LED-on-condition
	led.state           = LED_STATE_BREATHING;
	led.breathing_time  = 0;
	led.breathing_index = 255;
	led.breathing_up    = false;
}

void led_set_blinking(const uint8_t num) {
	// Check if we are already blinking with the correct blink amount
	if((led.state == LED_STATE_BLINKING) && (led.blink_num == num)) {
		return;
	}

	// Blinking overwrites API configuration
	if(led.state == LED_STATE_API) {
		led_reset_api_state();
	}

	led.state           = LED_STATE_BLINKING;
	led.blink_num       = num;
	led.blink_count     = num;
	led.blink_on        = false;
	led.blink_last_time = system_timer_get_ms();

	led_update(0, 0, 0);
}

// Called whenever there is activity
// LED will go to standby after 15 minutes again
void led_set_on(const bool force) {
	// led "on" does not overwrite API state
	if(!force && (led.state == LED_STATE_API)) {
		return;
	}

	led_hsv_to_rgb(led.h, led.s, led.v, &led.r, &led.g, &led.b);
	led.on_time = system_timer_get_ms();
	led.state = LED_STATE_ON;
	led_update(LED_HUE_STANDARD, 255, 255);
}

void led_set_off(void) {
	// led "off" does not overwrite API state
	if(led.state == LED_STATE_API) {
		return;
	}

	led.state = LED_STATE_OFF;
	led_update(0, 0, 0);
}

void led_init_v2(void) {
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
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_HIGH,
		.timer_concatenation = 0
	};

	const XMC_GPIO_CONFIG_t gpio_out_config	= {
		.mode                = XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT9,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

    XMC_CCU4_Init(EVSE_V2_LED_CCU, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
    XMC_CCU4_StartPrescaler(EVSE_V2_LED_CCU);
    XMC_CCU4_SLICE_CompareInit(EVSE_V2_LED_SLICE, &compare_config);

    // Set the period and compare register values
    XMC_CCU4_SLICE_SetTimerPeriodMatch(EVSE_V2_LED_SLICE, LED_MAX_DUTY_CYCLE-1);
    XMC_CCU4_SLICE_SetTimerCompareMatch(EVSE_V2_LED_SLICE, 0);

    XMC_CCU4_EnableShadowTransfer(EVSE_V2_LED_CCU, (XMC_CCU4_SHADOW_TRANSFER_SLICE_0 << (EVSE_V2_LED_SLICE_NUMBER*4)) |
    		                             (XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_0 << (EVSE_V2_LED_SLICE_NUMBER*4)));

    XMC_GPIO_Init(EVSE_V2_LED_PIN, &gpio_out_config);

    XMC_CCU4_EnableClock(EVSE_V2_LED_CCU, EVSE_V2_LED_SLICE_NUMBER);
    XMC_CCU4_SLICE_StartTimer(EVSE_V2_LED_SLICE);
}

void led_init_v3(void) {
	XMC_CCU8_SLICE_COMPARE_CONFIG_t compare_config = {
		.timer_mode          = XMC_CCU8_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU8_SLICE_PRESCALER_MODE_NORMAL,
		.prescaler_initval   = 0,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level_out0  = XMC_CCU8_SLICE_OUTPUT_PASSIVE_LEVEL_HIGH,
		.passive_level_out1  = XMC_CCU8_SLICE_OUTPUT_PASSIVE_LEVEL_HIGH,
		.passive_level_out2  = XMC_CCU8_SLICE_OUTPUT_PASSIVE_LEVEL_HIGH,
		.passive_level_out3  = XMC_CCU8_SLICE_OUTPUT_PASSIVE_LEVEL_HIGH,
		.timer_concatenation = 0
	};

	XMC_CCU8_Init(EVSE_V3_LED_R_CCU, XMC_CCU8_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU8_Init(EVSE_V3_LED_G_CCU, XMC_CCU8_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU8_Init(EVSE_V3_LED_B_CCU, XMC_CCU8_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU8_StartPrescaler(EVSE_V3_LED_R_CCU);
	XMC_CCU8_StartPrescaler(EVSE_V3_LED_G_CCU);
	XMC_CCU8_StartPrescaler(EVSE_V3_LED_B_CCU);
	compare_config.passive_level_out0 = EVSE_V3_LED_R_PASSIVE_LEVEL;
	compare_config.passive_level_out1 = EVSE_V3_LED_R_PASSIVE_LEVEL;
	compare_config.passive_level_out2 = EVSE_V3_LED_R_PASSIVE_LEVEL;
	compare_config.passive_level_out3 = EVSE_V3_LED_R_PASSIVE_LEVEL;
	XMC_CCU8_SLICE_CompareInit(EVSE_V3_LED_R_SLICE, &compare_config);
	compare_config.passive_level_out0 = EVSE_V3_LED_G_PASSIVE_LEVEL;
	compare_config.passive_level_out1 = EVSE_V3_LED_G_PASSIVE_LEVEL;
	compare_config.passive_level_out2 = EVSE_V3_LED_G_PASSIVE_LEVEL;
	compare_config.passive_level_out3 = EVSE_V3_LED_G_PASSIVE_LEVEL;
	XMC_CCU8_SLICE_CompareInit(EVSE_V3_LED_G_SLICE, &compare_config);
	compare_config.passive_level_out0 = EVSE_V3_LED_B_PASSIVE_LEVEL;
	compare_config.passive_level_out1 = EVSE_V3_LED_B_PASSIVE_LEVEL;
	compare_config.passive_level_out2 = EVSE_V3_LED_B_PASSIVE_LEVEL;
	compare_config.passive_level_out3 = EVSE_V3_LED_B_PASSIVE_LEVEL;
	XMC_CCU8_SLICE_CompareInit(EVSE_V3_LED_B_SLICE, &compare_config);

	// Set the period and compare register values
	XMC_CCU8_SLICE_SetTimerPeriodMatch(EVSE_V3_LED_R_SLICE, LED_MAX_DUTY_CYCLE-1);
	XMC_CCU8_SLICE_SetTimerPeriodMatch(EVSE_V3_LED_G_SLICE, LED_MAX_DUTY_CYCLE-1);
	XMC_CCU8_SLICE_SetTimerPeriodMatch(EVSE_V3_LED_B_SLICE, LED_MAX_DUTY_CYCLE-1);
	XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_R_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, 0); // TODO: which compare channel?
	XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_G_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, 0);
	XMC_CCU8_SLICE_SetTimerCompareMatch(EVSE_V3_LED_B_SLICE, XMC_CCU8_SLICE_COMPARE_CHANNEL_1, 0);

	XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_R_CCU, EVSE_V3_LED_R_SHADOW);
	XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_G_CCU, EVSE_V3_LED_G_SHADOW);
	XMC_CCU8_EnableShadowTransfer(EVSE_V3_LED_B_CCU, EVSE_V3_LED_B_SHADOW);

	XMC_GPIO_CONFIG_t gpio_out_config = {
		.mode                = EVSE_V3_LED_R_ALT,
		.input_hysteresis    = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		.output_level        = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};
    XMC_GPIO_Init(EVSE_V3_LED_R_PIN, &gpio_out_config);
	gpio_out_config.mode = EVSE_V3_LED_G_ALT;
    XMC_GPIO_Init(EVSE_V3_LED_G_PIN, &gpio_out_config);
	gpio_out_config.mode = EVSE_V3_LED_B_ALT;
    XMC_GPIO_Init(EVSE_V3_LED_B_PIN, &gpio_out_config);

	XMC_CCU8_EnableClock(EVSE_V3_LED_R_CCU, EVSE_V3_LED_R_SLICE_NUMBER);
	XMC_CCU8_EnableClock(EVSE_V3_LED_G_CCU, EVSE_V3_LED_G_SLICE_NUMBER);
	XMC_CCU8_EnableClock(EVSE_V3_LED_B_CCU, EVSE_V3_LED_B_SLICE_NUMBER);
	XMC_CCU8_SLICE_StartTimer(EVSE_V3_LED_R_SLICE);
	XMC_CCU8_SLICE_StartTimer(EVSE_V3_LED_G_SLICE);
	XMC_CCU8_SLICE_StartTimer(EVSE_V3_LED_B_SLICE);
}

void led_init(void) {
	memset(&led, 0, sizeof(LED));

	led_hsv_to_rgb(led.h, led.s, led.v, &led.r, &led.g, &led.b);

	if(hardware_version.is_v2) {
		led_init_v2();
	} else if(hardware_version.is_v3) {
		led_init_v3();
	}

	led_update(0, 0, 0);
	led_reset_api_state();

	led.state = LED_STATE_FLICKER;
}

void led_tick_status_off(void) {
	led_update(0, 0, 0);
}

void led_tick_status_on(void) {
	if(system_timer_is_time_elapsed_ms(led.on_time, LED_STANDBY_TIME)) {
		led.state = LED_STATE_OFF;
		led_update(0, 0, 0);
	} else {
		led_update(LED_HUE_STANDARD, 255, 255);
	}
}

void led_tick_status_blinking(const bool is_external) {
	if(led.blink_count >= led.blink_num) {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_WAIT)) {
			led.blink_last_time         = system_timer_get_ms();
			led.blink_count             = 0;
			led.currently_in_wait_state = true;
		}
	} else if(led.blink_on) {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_ON)) {
			led.blink_last_time = system_timer_get_ms();
			led_update(0, 0, 0);

			led.blink_on = false;
			led.blink_count++;
		}
	} else {
		if(system_timer_is_time_elapsed_ms(led.blink_last_time, LED_BLINK_DURATION_OFF)) {
			led.blink_last_time = system_timer_get_ms();
			if(hardware_version.is_v2) {
				led_update(LED_HUE_STANDARD, 255, 255); // default v2
			} else if(is_external && (led.v != 0)) {
				led_update(led.h, led.s, led.v);        // custom v3 through API
			} else {
				led_update(LED_HUE_RED, 255, 255);      // default v3
			}
			led.blink_on = true;
		}
	}
}

void led_tick_status_flicker(void) {
	if(system_timer_is_time_elapsed_ms(led.flicker_last_time, LED_FLICKER_DURATION)) {
		if(led.flicker_on) {
			led_update(hardware_version.is_v2 ? LED_HUE_STANDARD : LED_HUE_BOOTING, 255, 255);
		} else {
			led_update(0, 0, 0);
		}
		led.flicker_last_time = system_timer_get_ms();
		led.flicker_on        = ! led.flicker_on;
	}
}

void led_tick_status_breathing(void) {
	if(!system_timer_is_time_elapsed_ms(led.breathing_time, 5)) {
		return;
	}
	led.breathing_time = system_timer_get_ms();

	if(led.breathing_up) {
		led.breathing_index += 1;
	} else {
		led.breathing_index -= 1;
	}
	led.breathing_index = BETWEEN(0, led.breathing_index, 255);
	
	if(led.breathing_index == 0) {
		led.breathing_up = true;
	} else if(led.breathing_index == 255) {
		led.breathing_up = false;
	}

	led_update(LED_HUE_STANDARD, 255, led.breathing_index);
}

void led_tick_status_api_ack(void) {
	if((led.api_ack_counter < 3) && (system_timer_is_time_elapsed_ms(led.api_ack_time, 2))) {
		led_update(led.h, led.s, led.api_ack_index);
		led.api_ack_index++;
		led.api_ack_time+=2;
		if(led.api_ack_index == 0) {
			led.api_ack_counter++;
		}
	} else if((led.api_ack_counter >= 3) && (system_timer_is_time_elapsed_ms(led.api_ack_time, 2000))) {
		led.api_ack_index           = 0;
		led.api_ack_counter         = 0;
		led.api_ack_time            = system_timer_get_ms();
		led.currently_in_wait_state = true;
	}
}

void led_tick_status_api_nack(void) {
	if((led.api_nack_counter < 1) && (system_timer_is_time_elapsed_ms(led.api_nack_time, 1))) {
		led_update(led.h, led.s, 255-led.api_nack_index);
		led.api_nack_index++;
		led.api_nack_time++;
		if(led.api_nack_index == 0) {
			led.api_nack_counter++;
		}
	} else if((led.api_nack_counter >= 1) && (system_timer_is_time_elapsed_ms(led.api_nack_time, 400))) {
		led.api_nack_index          = 0;
		led.api_nack_counter        = 0;
		led.api_nack_time           = system_timer_get_ms();
		led.currently_in_wait_state = true;
	}
}

void led_tick_status_api_nag(void) {
	if((led.api_nag_counter == 0) && (system_timer_is_time_elapsed_ms(led.api_nag_time, 1))) {
		led_update(led.h, led.s, led.api_nag_index);
		led.api_nag_index++;
		led.api_nag_time++;
		if(led.api_nag_index == 0) {
			led.api_nag_counter++;
		}
	} else if((led.api_nag_counter == 1) && (system_timer_is_time_elapsed_ms(led.api_nag_time, 1))) {
		led_update(led.h, led.s, 255-led.api_nag_index);
		led.api_nag_index++;
		led.api_nag_time++;
		if(led.api_nag_index == 0) {
			led.api_nag_counter++;
		}
	} else if((led.api_nag_counter >= 2) && (system_timer_is_time_elapsed_ms(led.api_nag_time, 400))) {
		led.api_nag_index           = 0;
		led.api_nag_counter         = 0;
		led.api_nag_time            = system_timer_get_ms();
		led.currently_in_wait_state = true;
	}
}

void led_tick_status_api(void) {
	led.currently_in_wait_state = false;

	if((led.api_indication >= 0) && (led.api_indication <= 255)) {
		led_update(led.h, led.s, led.api_indication);
	} else if(led.api_indication == 1001) {
		led_tick_status_api_ack();
	} else if(led.api_indication == 1002) {
		led_tick_status_api_nack();
	} else if(led.api_indication == 1003) {
		led_tick_status_api_nag();
	} else if(led.api_indication > 2000 && led.api_indication < 2011) {
		if(led.blink_external != led.api_indication) {
			// If external blinking is activated, turn LED off and start in off state
			// with last time set to now.
			// This way we don't waste any time and the first blinking pattern already has
			// the correct amount of blinks.
			led_update(0, 0, 0);
			led.blink_num       = led.api_indication - 2000;
			led.blink_count     = 0;
			led.blink_on        = false;
			led.blink_last_time = system_timer_get_ms();
			led.blink_external  = led.api_indication;
		}
		led_tick_status_blinking(true);
	}

	if(system_timer_is_time_elapsed_ms(led.api_start, led.api_duration) && !led.currently_in_wait_state) {
		led_reset_api_state();
		led_set_on(true);
		led.blink_external = -1;
	}
}

// TODO: Always use HSV. Use V for dimming (instead of index) and then the cie1931 table for the resulting rgb value.
void led_tick(void) {
	if((led.state != LED_STATE_API) || (led.api_indication <= 2000) || (led.api_indication >= 2011)) {
		led.blink_external  = -1;
	}

	switch(led.state) {
		case LED_STATE_OFF:       led_tick_status_off();            break;
		case LED_STATE_ON:        led_tick_status_on();             break;
		case LED_STATE_BLINKING:  led_tick_status_blinking(false);  break;
		case LED_STATE_FLICKER:   led_tick_status_flicker();        break;
		case LED_STATE_BREATHING: led_tick_status_breathing();      break;
		case LED_STATE_API:       led_tick_status_api();            break;
	}
}
