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

// Resistance between PP/PE
// 1000..2200 Ohm => 13A
// 330..1000 Ohm  => 20A
// 150..330 Ohm   => 32A
// 75..150 Ohm    => 63A
#define IEC61851_PP_RESISTANCE_13A 1000
#define IEC61851_PP_RESISTANCE_20A  330
#define IEC61851_PP_RESISTANCE_32A  150

typedef enum {
	IEC61851_STATE_A  = 0, // Standby
	IEC61851_STATE_B  = 1, // Vehicle Detected
	IEC61851_STATE_C  = 2, // Ready (Charging)
	IEC61851_STATE_D  = 3, // Ready with ventilation
	IEC61851_STATE_EF = 4, // No Power / Error
} IEC61851State;


typedef struct {
	IEC61851State state;
	uint32_t last_state_change;
	uint32_t boot_time;

	uint8_t diode_error_counter;
	uint8_t diode_ok_counter;
	bool diode_check_pending;
	uint16_t diode_vcp1_last_count;
	uint16_t diode_vcp2_last_count;
	uint8_t diode_vcp1_seen;
	uint8_t diode_vcp2_seen;

	uint32_t last_error_time;
	uint32_t last_state_c_end_time;

	uint32_t state_b1b2_transition_time;
	uint32_t state_b1b2_transition_seen;

	bool currently_beeing_woken_up;
	bool instant_phase_switch_allowed;

	uint32_t time_in_b2;

	bool iso15118_active;
	uint16_t iso15118_cp_duty_cycle;

	bool force_state_f;
	uint32_t force_state_f_time;
} IEC61851;

extern IEC61851 iec61851;

void iec61851_init(void);
void iec61851_tick(void);

uint32_t iec61851_get_ma_from_pp_resistance(void);
uint32_t iec61851_get_max_ma(void);
float iec61851_get_duty_cycle_for_ma(uint32_t ma);
void iec61851_reset_ev_wakeup(void);
void iec61851_set_state(IEC61851State state);
uint16_t iec61851_get_cp_resistance_threshold(IEC61851State transition_to_state);

#endif
