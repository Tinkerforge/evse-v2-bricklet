/* evse-v2-bricklet
 * Copyright (C) 2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * charging_slot.h: Charging slot implementation
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

#ifndef CHARGING_SLOT_H
#define CHARGING_SLOT_H

#include <stdint.h>
#include <stdbool.h>

#define CHARGING_SLOT_NUM 20

#define CHARGING_SLOT_INCOMING_CABLE 0
#define CHARGING_SLOT_OUTGOING_CABLE 1
#define CHARGING_SLOT_INPUT0         2
#define CHARGING_SLOT_INPUT1         3
#define CHARGING_SLOT_BUTTON         4

typedef struct {
    uint16_t max_current[CHARGING_SLOT_NUM];
    bool active[CHARGING_SLOT_NUM];
    bool clear_on_disconnect[CHARGING_SLOT_NUM];
} ChargingSlot;

extern ChargingSlot charging_slot;

void charging_slot_init(void);
void charging_slot_tick(void);
uint16_t charging_slot_get_max_current(void);
void charging_slot_start_charging_by_button(void);
void charging_slot_stop_charging_by_button(void);
void charging_slot_handle_disconnect(void);
#endif