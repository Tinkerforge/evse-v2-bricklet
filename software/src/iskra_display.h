/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iskra_display.h: Custom text and backlight control for Iskra WM3M4(C) LCD
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

#ifndef ISKRA_DISPLAY_H
#define ISKRA_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define ISKRA_DISPLAY_TEXT_LENGTH  8
#define ISKRA_DISPLAY_LABEL_LENGTH 4

#define ISKRA_DISPLAY_BACKLIGHT_AUTO_OFF_TIME (5*60*1000)

typedef struct {
    char text[ISKRA_DISPLAY_TEXT_LENGTH];
    char label[ISKRA_DISPLAY_LABEL_LENGTH];
    bool text_pending;

    uint8_t backlight_mode;
    bool backlight_desired;
    bool backlight_written;
    bool backlight_written_valid;

    bool charging_last;
    bool button_pressed_last;
    uint32_t event_time;

    uint8_t meter_type_last;

    uint8_t state;
    uint32_t state_time;
} IskraDisplay;

extern IskraDisplay iskra_display;

void iskra_display_init(void);
void iskra_display_tick(void);
bool iskra_display_has_work(void);
void iskra_display_modbus_tick(void);

#endif
