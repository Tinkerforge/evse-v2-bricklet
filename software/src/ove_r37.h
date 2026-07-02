/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ove_r37.h: OVE Richtlinie R 37 (2024-12-01) grid support tests
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

#ifndef OVE_R37_H
#define OVE_R37_H

#include <stdint.h>
#include <stdbool.h>

#define OVE_R37_NOMINAL_VOLTAGE_MV               230000
#define OVE_R37_METER_STALE_MS                   1000

// 5.9.8 Unterspannungsauslösung:
#define OVE_R37_UNDERVOLTAGE_TRIP_PU_DEFAULT     800
#define OVE_R37_UNDERVOLTAGE_OBSERVE_MS_DEFAULT  3000

// 5.9.9 Spannungsbereiche
#define OVE_R37_VOLTAGE_RANGE_MIN_PU             900
#define OVE_R37_VOLTAGE_RANGE_MAX_PU             1100

// 5.7.4.2 Wiederzuschaltung
#define OVE_R37_RECONNECT_VOLTAGE_MIN_PU         900    // 0.9 pu
#define OVE_R37_RECONNECT_VOLTAGE_MAX_PU         1090   // 1.09 pu
#define OVE_R37_RECONNECT_FREQUENCY_MIN_MHZ      49900  // 49.90 Hz
#define OVE_R37_RECONNECT_FREQUENCY_MAX_MHZ      50100  // 50.10 Hz

// 5.7.4.2 Wartezeit
#define OVE_R37_RECONNECT_WAIT_S_DEFAULT         60
#define OVE_R37_RECONNECT_WAIT_S_MAX             300

// 5.7.4.2 Hochlauframpe
#define OVE_R37_RAMP_START_MA                    6000
#define OVE_R37_RAMP_FULL_MA                     32000
#define OVE_R37_RAMP_RATE_MA_PER_MIN             1000   // 1 A/min

// 5.9.3 Symmetriebedingungen
#define OVE_R37_SYMMETRY_MAX_DIFF_MA             16000
#define OVE_R37_SYMMETRY_RELEASE_DIFF_MA         14000
#define OVE_R37_SYMMETRY_REACTION_MS             60000

#define OVE_R37_NO_LIMIT                         0xFFFF

// 5.9.2 Prüfung B
#define OVE_R37_START_DELAY_S_MAX                300

#define OVE_R37_TRIP_NONE                        0
#define OVE_R37_TRIP_UNDERVOLTAGE                (1 << 0)
#define OVE_R37_TRIP_OVERVOLTAGE                 (1 << 1)
#define OVE_R37_TRIP_FREQUENCY                   (1 << 2)

#define OVE_R37_FLAG_VOLTAGE_IN_RANGE            (1 << 0)
#define OVE_R37_FLAG_FREQUENCY_IN_RANGE          (1 << 1)
#define OVE_R37_FLAG_VOLTAGE_VALID               (1 << 2)
#define OVE_R37_FLAG_CURRENT_VALID               (1 << 3)
#define OVE_R37_FLAG_FREQUENCY_VALID             (1 << 4)

#define OVE_R37_BOOT_WINDOW_MS                   30000

typedef enum {
	OVE_R37_STATE_DISABLED = 0,
	OVE_R37_STATE_NORMAL,
	OVE_R37_STATE_TRIPPED,
	OVE_R37_STATE_WAIT,
	OVE_R37_STATE_RAMP,
	OVE_R37_STATE_BOOT,
} OveR37State;

typedef struct {
	bool enabled;
	uint16_t undervoltage_threshold_pu; // 1/1000 pu, default 800 (5.9.8)
	uint16_t undervoltage_observe_ms;   // observation time before trip, default 3000 (5.9.8)
	uint16_t reconnect_wait_s;          // tautom, default 60, max 300 (5.7.4.2)
	uint16_t start_delay_s;             // charge start delay, max 300 (5.9.2 B)

	// Measurement inputs
	uint32_t voltage[3];
	uint32_t current[3];
	bool phase_connected[3];
	bool phase_monitored[3];

	bool voltage_valid;
	bool current_valid;
	uint32_t frequency;
	bool frequency_valid;
	bool charge_requested;

	// Runtime state machine.
	OveR37State state;
	uint8_t trip_reason;

	// Status flags
	bool voltage_in_range;
	bool frequency_in_range;

	// Timers
	uint32_t undervoltage_since;
	uint32_t wait_start;
	uint32_t ramp_start;
	uint32_t boot_start;
	uint32_t start_delay_ref;
	bool charge_requested_last;
	bool start_delay_active;

	// Intermediate per-test current limits
	uint16_t symmetry_limit; // 5.9.3
	uint16_t ramp_limit;     // 5.7.4.2

	// Result
	uint16_t max_current;
	bool slot_active;
} OveR37;

extern OveR37 ove_r37;

void ove_r37_init(void);
void ove_r37_tick(void);

void ove_r37_check_undervoltage_trip(void);
void ove_r37_check_voltage_range(void);
void ove_r37_check_frequency_range(void);
void ove_r37_check_boot_lockout(void);
void ove_r37_check_reconnect_conditions(void);
void ove_r37_apply_power_ramp(void);
void ove_r37_check_phase_symmetry(void);
void ove_r37_apply_start_delay(void);
void ove_r37_update_charging_slot(void);

uint8_t ove_r37_get_flags(void);

#endif
