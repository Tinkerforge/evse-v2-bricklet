/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led.h: EVSE 2.0 LED driver
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

#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_FLICKER_DURATION    50

#define LED_BLINK_DURATION_ON   250
#define LED_BLINK_DURATION_OFF  250
#define LED_BLINK_DURATION_WAIT 2000

#define LED_STANDBY_TIME (1000*60*15) // Standby after 15 minutes

typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINKING,
    LED_STATE_FLICKER,
    LED_STATE_BREATHING
} LEDState;

typedef struct {
    LEDState state;

    uint32_t on_time;

    uint32_t blink_num;
    uint32_t blink_count;
	bool blink_on;
	uint32_t blink_last_time;

    bool flicker_on;
    uint32_t flicker_last_time;
} LED;

extern LED led;

void led_set_on(void);
void led_set_blinking(const uint8_t num);
void led_init();
void led_tick();

#endif