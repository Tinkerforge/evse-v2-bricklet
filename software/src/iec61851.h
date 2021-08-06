/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iec61851.h: Implementation of IEC 61851 EVSE state machine
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

#ifndef IEC61851_H
#define IEC61851_H

#include <stdint.h>

typedef enum {
    IEC61851_STATE_A,  // Standby
    IEC61851_STATE_B,  // Vehicle Detected
    IEC61851_STATE_C,  // Ready (Charging)
    IEC61851_STATE_D,  // Ready with ventilation
    IEC61851_STATE_EF, // No Power / Error
} IEC61851State;


typedef struct {
    IEC61851State state;
    uint32_t last_state_change;

    uint8_t diode_error_counter;
} IEC61851;

extern IEC61851 iec61851;

void iec61851_init(void);
void iec61851_tick(void);

uint32_t iec61851_get_ma_from_pp_resistance(void);
uint32_t iec61851_get_ma_from_jumper(void);
uint32_t iec61851_get_max_ma(void);
uint16_t iec61851_get_duty_cycle_for_ma(uint32_t ma);

#endif