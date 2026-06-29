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

// Nominal phase-to-neutral mains voltage (Un) in mV, used as the 1.0 pu reference.
#define OVE_R37_NOMINAL_VOLTAGE_MV               230000

// Meter measurements older than this (no successful fast read) are treated as
// invalid, so the checks degrade safely instead of acting on stale data.
#define OVE_R37_METER_STALE_MS                   1000

// Per-unit thresholds expressed in 1/1000 pu (1000 = 1.0 pu = Un).

// 5.9.8 Unterspannungsauslösung: default trip below 0.80 pu for longer than 3 s.
#define OVE_R37_UNDERVOLTAGE_TRIP_PU_DEFAULT     800
#define OVE_R37_UNDERVOLTAGE_OBSERVE_MS_DEFAULT  3000

// 5.9.9 Spannungsbereiche: continuous operation must be kept within 0.9..1.1 pu.
#define OVE_R37_VOLTAGE_RANGE_MIN_PU             900
#define OVE_R37_VOLTAGE_RANGE_MAX_PU             1100

// 5.1.1 Frequenzbereiche: must not disconnect within 47.6..51.4 Hz (milli-Hz).
#define OVE_R37_FREQUENCY_RANGE_MIN_MHZ          47600
#define OVE_R37_FREQUENCY_RANGE_MAX_MHZ          51400

// 5.7.4.2 Wiederzuschaltung: reconnect supply window.
#define OVE_R37_RECONNECT_VOLTAGE_MIN_PU         900    // 0.9 pu
#define OVE_R37_RECONNECT_VOLTAGE_MAX_PU         1090   // 1.09 pu
#define OVE_R37_RECONNECT_FREQUENCY_MIN_MHZ      49900  // 49.90 Hz
#define OVE_R37_RECONNECT_FREQUENCY_MAX_MHZ      50100  // 50.10 Hz

// 5.7.4.2 Wartezeit tautom: settable 0..300 s, default 60 s.
#define OVE_R37_RECONNECT_WAIT_S_DEFAULT         60
#define OVE_R37_RECONNECT_WAIT_S_MAX             300

// 5.7.4.2 Hochlauframpe: ramp the offered current up after reconnect. The
// standard permits AC stations to use a 1 A/min current ramp as an alternative
// to 10 % of rated power per minute; we use the simpler, always-compliant
// 1 A/min ramp starting at the technical minimum current.
#define OVE_R37_RAMP_START_MA                    6000   // technical minimum (6 A)
#define OVE_R37_RAMP_FULL_MA                     32000  // ramp target / max EVSE current
#define OVE_R37_RAMP_STEP_MA                     1000   // 1 A per step
#define OVE_R37_RAMP_STEP_MS                     60000  // every 60 s -> 1 A/min

// 5.9.3 Symmetriebedingungen: keep max pairwise phase current difference
// <= 16 A, reaction time <= 60 s. When violated the offered current is capped
// to 16 A; a hysteresis avoids oscillation around the threshold.
#define OVE_R37_SYMMETRY_MAX_DIFF_MA             16000
#define OVE_R37_SYMMETRY_RELEASE_DIFF_MA         14000
#define OVE_R37_SYMMETRY_REACTION_MS             60000

// Sentinel for "no current limit imposed" (max_current is uint16_t mA).
#define OVE_R37_NO_LIMIT                         0xFFFF

// 5.9.2 Prüfung B: charge start delay, settable 0..300 s.
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

typedef enum {
	OVE_R37_STATE_DISABLED = 0,
	OVE_R37_STATE_NORMAL,
	OVE_R37_STATE_TRIPPED,
	OVE_R37_STATE_WAIT,
	OVE_R37_STATE_RAMP,
} OveR37State;

typedef struct {
	bool enabled;                       // R37 mode active (country = AT)
	uint16_t undervoltage_threshold_pu; // 1/1000 pu, default 800 (5.9.8)
	uint16_t undervoltage_observe_ms;   // observation time before trip, default 3000 (5.9.8)
	uint16_t reconnect_wait_s;          // tautom, default 60, max 300 (5.7.4.2)
	uint16_t start_delay_s;             // charge start delay, max 300 (5.9.2 B)

	// Measurement inputs (written by external code in a later phase; the meter
	// provides per-phase voltage/current, frequency.c provides the frequency).
	uint32_t voltage[3];                // phase-to-neutral voltage L1/L2/L3 in mV
	uint32_t current[3];                // phase current L1/L2/L3 in mA
	bool phase_connected[3];            // whether each phase is currently energized (meter live flag, >180 V)
	bool phase_monitored[3];            // latched "phase is part of the installation"; stays set when a
	                                    // present phase sags below the meter's connected threshold (5.9.8/5.7.4.2)
	bool voltage_valid;                 // voltage[] and phase_connected[] are fresh and valid
	bool current_valid;                 // current[] is fresh and valid
	uint32_t frequency;                 // mains frequency in milli-Hz (50000 = 50.00 Hz)
	bool frequency_valid;               // frequency is fresh and valid
	bool charge_requested;              // vehicle connected and requesting to charge (5.9.2 B)

	// Runtime state machine.
	OveR37State state;
	uint8_t trip_reason;

	// Status flags
	bool voltage_in_range;              // all connected phases within 0.9..1.1 pu
	bool frequency_in_range;            // frequency within 47.6..51.4 Hz

	// Timers
	uint32_t undervoltage_since;        // first ms a phase fell below the threshold
	uint32_t wait_start;                // ms the reconnect wait was started
	uint32_t ramp_start;                // ms the post-reconnect ramp was started
	uint32_t start_delay_ref;           // ms a charge was requested (start delay reference)
	bool charge_requested_last;         // previous charge_requested (rising-edge detection)
	bool start_delay_active;            // charge start currently held off by the delay

	// Intermediate per-test current limits
	uint16_t symmetry_limit;            // 5.9.3
	uint16_t ramp_limit;                // 5.7.4.2

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
void ove_r37_check_reconnect_conditions(void);
void ove_r37_apply_power_ramp(void);
void ove_r37_check_phase_symmetry(void);
void ove_r37_apply_start_delay(void);
void ove_r37_update_charging_slot(void);

uint8_t ove_r37_get_flags(void);

#endif
