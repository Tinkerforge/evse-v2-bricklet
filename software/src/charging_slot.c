/* evse-v2-bricklet
 * Copyright (C) 2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * charging_slot.c: Charging slot implementation
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

#include "charging_slot.h"

#include <string.h>

#include "bricklib2/utility/util_definitions.h"

#include "button.h"
#include "communication.h"
#include "evse.h"
#include "iec61851.h"

ChargingSlot charging_slot;

uint32_t charging_slot_get_ma_incoming_cable(void) {
	switch(evse.config_jumper_current) {
		case EVSE_CONFIG_JUMPER_CURRENT_6A:  return 6000;
		case EVSE_CONFIG_JUMPER_CURRENT_10A: return 10000;
		case EVSE_CONFIG_JUMPER_CURRENT_13A: return 13000;
		case EVSE_CONFIG_JUMPER_CURRENT_16A: return 16000;
		case EVSE_CONFIG_JUMPER_CURRENT_20A: return 20000;
		case EVSE_CONFIG_JUMPER_CURRENT_25A: return 25000;
		case EVSE_CONFIG_JUMPER_CURRENT_32A: return 32000;
		case EVSE_CONFIG_JUMPER_SOFTWARE: return evse.config_jumper_current_software;
		default: return 6000;
	}
}

void charging_slot_init(void) {
    memset(&charging_slot, 0, sizeof(ChargingSlot));

    // Incoming cable
    charging_slot.max_current[CHARGING_SLOT_INCOMING_CABLE]         = charging_slot_get_ma_incoming_cable();
    charging_slot.active[CHARGING_SLOT_INCOMING_CABLE]              = true;
    charging_slot.clear_on_disconnect[CHARGING_SLOT_INCOMING_CABLE] = false;

    // Outgoing cable
    charging_slot.max_current[CHARGING_SLOT_OUTGOING_CABLE]         = iec61851_get_ma_from_pp_resistance();
    charging_slot.active[CHARGING_SLOT_OUTGOING_CABLE]              = true;
    charging_slot.clear_on_disconnect[CHARGING_SLOT_OUTGOING_CABLE] = false;

    // Input 0 (shutdown input)
    charging_slot.max_current[CHARGING_SLOT_INPUT0]                 = 0;
    charging_slot.active[CHARGING_SLOT_INPUT0]                      = false;
    charging_slot.clear_on_disconnect[CHARGING_SLOT_INPUT0]         = false;

    // Input 1 (gp input)
    charging_slot.max_current[CHARGING_SLOT_INPUT1]                 = 0;
    charging_slot.active[CHARGING_SLOT_INPUT1]                      = false;
    charging_slot.clear_on_disconnect[CHARGING_SLOT_INPUT1]         = false;

    // Button -> default is autostart enabled
    charging_slot.max_current[CHARGING_SLOT_BUTTON]                 = 32000;
    charging_slot.active[CHARGING_SLOT_BUTTON]                      = true;
    charging_slot.clear_on_disconnect[CHARGING_SLOT_BUTTON]         = false;


    // TODO: Read defaults
}

void charging_slot_tick(void) {
    charging_slot.max_current[CHARGING_SLOT_OUTGOING_CABLE] = iec61851_get_ma_from_pp_resistance();
}

uint16_t charging_slot_get_max_current(void) {
    uint16_t max_current = 0xFFFF;

    for(uint8_t i = 0; i < CHARGING_SLOT_NUM; i++) {
        if(charging_slot.active[i]) {
            max_current = MIN(max_current, charging_slot.max_current[i]);
        }
    }

    if(max_current == 0xFFFF) {
        return 0;
    }

    return max_current;
}

void charging_slot_handle_disconnect(void) {
    for(uint8_t i = 0; i < CHARGING_SLOT_NUM; i++) {
        if(charging_slot.clear_on_disconnect[i]) {
            charging_slot.max_current[i] = 0;
        }
    }

    // If the charging was stopped because of a button press,
    // we set the button-slot back on disconnect
    if(button.was_pressed && (button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING)) {
        charging_slot_start_charging_by_button();
    }
}

void charging_slot_stop_charging_by_button(void) {
    charging_slot.max_current[CHARGING_SLOT_BUTTON] = 0;
}

void charging_slot_start_charging_by_button(void) {
    charging_slot.max_current[CHARGING_SLOT_BUTTON] = 32000;
}