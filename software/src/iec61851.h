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
#include <stdbool.h>

// Resistance between CP/PE
// inf  Ohm -> no car present
// 2700 Ohm -> car present
//  880 Ohm -> car charging
//  240 Ohm -> car charging with ventilation
// ==>
// > 10000 -> State A
// >  1790 -> State B
// >  560 -> State C
// >  150 -> State D
// <  150 -> State E/F
#define IEC61851_CP_RESISTANCE_STATE_A 10000
#define IEC61851_CP_RESISTANCE_STATE_B  1790
#define IEC61851_CP_RESISTANCE_STATE_C   300
#define IEC61851_CP_RESISTANCE_STATE_D   150

// Resistance between PP/PE
// 1000..2200 Ohm => 13A
// 330..1000 Ohm  => 20A
// 150..330 Ohm   => 32A
// 75..150 Ohm    => 63A
#define IEC61851_PP_RESISTANCE_13A 1000
#define IEC61851_PP_RESISTANCE_20A  330
#define IEC61851_PP_RESISTANCE_32A  150

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
	uint32_t last_error_time;

	uint32_t wait_after_cp_disconnect;

	uint32_t state_b1b2_transition_time;
	uint32_t state_b1b2_transition_disconnect_time;
	bool state_b1b2_transition_done;
} IEC61851;

extern IEC61851 iec61851;

void iec61851_init(void);
void iec61851_tick(void);

uint32_t iec61851_get_ma_from_pp_resistance(void);
uint32_t iec61851_get_max_ma(void);
float iec61851_get_duty_cycle_for_ma(uint32_t ma);

#endif
