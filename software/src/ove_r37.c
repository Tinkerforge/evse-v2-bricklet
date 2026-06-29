/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ove_r37.c: OVE Richtlinie R 37 (2024-12-01) grid support tests
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

#include "ove_r37.h"

#include <string.h>
#include <math.h>

#include "charging_slot.h"
#include "hardware_version.h"
#include "iec61851.h"

#include "bricklib2/warp/meter.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"

OveR37 ove_r37;

static bool ove_r37_meter_is_iskra(void) {
	return (meter.type == METER_TYPE_WM3M4C) || (meter.type == METER_TYPE_WM3M4);
}

static void ove_r37_update_measurements(void) {
	const bool fresh = !system_timer_is_time_elapsed_ms(meter.register_fast_time, OVE_R37_METER_STALE_MS);
	const bool valid = hardware_version.is_v4 &&
	                   ove_r37_meter_is_iskra() &&
	                   meter.available &&
	                   fresh;

	if(!valid) {
		ove_r37.voltage_valid   = false;
		ove_r37.current_valid   = false;
		ove_r37.frequency_valid = false;
		// Drop the latched installation phases when the meter is gone, so a
		// fresh meter session re-latches from the actual phases present.
		ove_r37.phase_monitored[0] = false;
		ove_r37.phase_monitored[1] = false;
		ove_r37.phase_monitored[2] = false;
		return;
	}

	ove_r37.voltage[0] = (uint32_t)(meter_register_set.VoltageL1N.f * 1000.0f);
	ove_r37.voltage[1] = (uint32_t)(meter_register_set.VoltageL2N.f * 1000.0f);
	ove_r37.voltage[2] = (uint32_t)(meter_register_set.VoltageL3N.f * 1000.0f);

	ove_r37.current[0] = (uint32_t)(fabsf(meter_register_set.CurrentL1ImExSum.f) * 1000.0f);
	ove_r37.current[1] = (uint32_t)(fabsf(meter_register_set.CurrentL2ImExSum.f) * 1000.0f);
	ove_r37.current[2] = (uint32_t)(fabsf(meter_register_set.CurrentL3ImExSum.f) * 1000.0f);

	for(uint8_t i = 0; i < 3; i++) {
		ove_r37.phase_connected[i] = meter.phases_connected[i];
		if(meter.phases_connected[i]) {
			ove_r37.phase_monitored[i] = true;
		}
	}

	ove_r37.frequency = (uint32_t)(meter_register_set.FrequencyLAvg.f * 1000.0f);

	ove_r37.voltage_valid   = true;
	ove_r37.current_valid   = true;
	ove_r37.frequency_valid = true;
}

static uint32_t ove_r37_pu_to_mv(const uint16_t pu_milli) {
	return (uint32_t)(((uint64_t)OVE_R37_NOMINAL_VOLTAGE_MV * pu_milli) / 1000U);
}

static uint32_t ove_r37_now_ms(void) {
	const uint32_t now = system_timer_get_ms();
	return (now == 0) ? 1 : now;
}

// Reconnect supply condition (5.7.4.2): all connected phases within
// 0.9..1.09 pu and the frequency within 49.90..50.10 Hz.
static bool ove_r37_reconnect_supply_ok(void) {
	if(!ove_r37.voltage_valid || !ove_r37.frequency_valid) {
		return false;
	}

	const uint32_t v_min = ove_r37_pu_to_mv(OVE_R37_RECONNECT_VOLTAGE_MIN_PU);
	const uint32_t v_max = ove_r37_pu_to_mv(OVE_R37_RECONNECT_VOLTAGE_MAX_PU);

	bool any_phase = false;
	for(uint8_t i = 0; i < 3; i++) {
		if(ove_r37.phase_monitored[i]) {
			any_phase = true;
			if((ove_r37.voltage[i] < v_min) || (ove_r37.voltage[i] > v_max)) {
				return false;
			}
		}
	}

	if(!any_phase) {
		return false;
	}

	if((ove_r37.frequency < OVE_R37_RECONNECT_FREQUENCY_MIN_MHZ) ||
	   (ove_r37.frequency > OVE_R37_RECONNECT_FREQUENCY_MAX_MHZ)) {
		return false;
	}

	return true;
}

void ove_r37_init(void) {
	memset(&ove_r37, 0, sizeof(OveR37));

	if(!hardware_version.is_v4) {
		return;
	}

	ove_r37.enabled                   = false;
	ove_r37.undervoltage_threshold_pu = OVE_R37_UNDERVOLTAGE_TRIP_PU_DEFAULT;
	ove_r37.undervoltage_observe_ms   = OVE_R37_UNDERVOLTAGE_OBSERVE_MS_DEFAULT;
	ove_r37.reconnect_wait_s          = OVE_R37_RECONNECT_WAIT_S_DEFAULT;
	ove_r37.start_delay_s             = 0;

	ove_r37.state          = OVE_R37_STATE_DISABLED;
	ove_r37.trip_reason    = OVE_R37_TRIP_NONE;
	ove_r37.symmetry_limit = OVE_R37_NO_LIMIT;
	ove_r37.ramp_limit     = OVE_R37_NO_LIMIT;
	ove_r37.max_current    = 0;
	ove_r37.slot_active    = false;
}

void ove_r37_tick(void) {
	if(!hardware_version.is_v4) {
		return;
	}

	ove_r37_update_measurements();

	if(!ove_r37.enabled) {
		ove_r37.state          = OVE_R37_STATE_DISABLED;
		ove_r37.trip_reason    = OVE_R37_TRIP_NONE;
		ove_r37.symmetry_limit = OVE_R37_NO_LIMIT;
		ove_r37.ramp_limit     = OVE_R37_NO_LIMIT;
		ove_r37_update_charging_slot();
		return;
	}

	// A charge is requested once a vehicle is connected (IEC state B or C).
	// The rising edge (A -> B) starts the configurable start delay (5.9.2 B),
	// which holds off the X1 -> X2 (PWM) charge start.
	ove_r37.charge_requested = (iec61851.state == IEC61851_STATE_B) ||
	                           (iec61851.state == IEC61851_STATE_C);

	if(ove_r37.state == OVE_R37_STATE_DISABLED) {
		ove_r37.state = OVE_R37_STATE_NORMAL;
	}

	ove_r37_check_undervoltage_trip();
	ove_r37_check_voltage_range();
	ove_r37_check_frequency_range();
	ove_r37_check_reconnect_conditions();
	ove_r37_apply_power_ramp();
	ove_r37_check_phase_symmetry();
	ove_r37_apply_start_delay();

	// Apply the combined result.
	ove_r37_update_charging_slot();
}

// 5.9.8 Unterspannungsauslösung
void ove_r37_check_undervoltage_trip(void) {
	// Without valid voltage data we cannot evaluate the condition; do not let
	// the observation timer run so we never trip on stale/missing data.
	if(!ove_r37.voltage_valid) {
		ove_r37.undervoltage_since = 0;
		return;
	}

	const uint32_t threshold_mv = ove_r37_pu_to_mv(ove_r37.undervoltage_threshold_pu);

	bool below = false;
	for(uint8_t i = 0; i < 3; i++) {
		if(ove_r37.phase_monitored[i] && (ove_r37.voltage[i] < threshold_mv)) {
			below = true;
		}
	}

	if(below) {
		if(ove_r37.undervoltage_since == 0) {
			ove_r37.undervoltage_since = ove_r37_now_ms();
		}

		// Trip once the voltage has stayed below the threshold for the whole
		// observation time. With the default 3 s observation plus the immediate
		// reaction here the charge stops well within the required 10 s.
		if(system_timer_is_time_elapsed_ms(ove_r37.undervoltage_since, ove_r37.undervoltage_observe_ms)) {
			ove_r37.trip_reason |= OVE_R37_TRIP_UNDERVOLTAGE;
			if((ove_r37.state == OVE_R37_STATE_NORMAL) || (ove_r37.state == OVE_R37_STATE_RAMP)) {
				ove_r37.state = OVE_R37_STATE_TRIPPED;
			}
		}
	} else {
		ove_r37.undervoltage_since = 0;
		ove_r37.trip_reason &= (uint8_t)~OVE_R37_TRIP_UNDERVOLTAGE;
	}
}

// 5.9.9 Spannungsbereiche
void ove_r37_check_voltage_range(void) {
	// Ride-through requirement: charging must not be interrupted while all
	// connected phases stay within 0.9..1.1 pu. We only compute a status flag
	// here; no trip is triggered inside this band (the undervoltage trip at
	// 0.8 pu lies below it, so it never fires within the allowed range).
	if(!ove_r37.voltage_valid) {
		ove_r37.voltage_in_range = false;
		return;
	}

	const uint32_t v_min = ove_r37_pu_to_mv(OVE_R37_VOLTAGE_RANGE_MIN_PU);
	const uint32_t v_max = ove_r37_pu_to_mv(OVE_R37_VOLTAGE_RANGE_MAX_PU);

	bool any_phase = false;
	bool in_range  = true;
	for(uint8_t i = 0; i < 3; i++) {
		if(ove_r37.phase_connected[i]) {
			any_phase = true;
			if((ove_r37.voltage[i] < v_min) || (ove_r37.voltage[i] > v_max)) {
				in_range = false;
			}
		}
	}

	ove_r37.voltage_in_range = any_phase && in_range;
}

// 5.1.1 Frequenzbereiche
void ove_r37_check_frequency_range(void) {
	// Ride-through requirement: the station must not disconnect while the
	// frequency stays within 47.6..51.4 Hz. As loads/charging stations have no
	// active LFSM requirement yet (5.1.2.3), the frequency never triggers a
	// trip; it only gates reconnection (5.7.4.2). We compute a status flag only.
	if(!ove_r37.frequency_valid) {
		ove_r37.frequency_in_range = false;
		return;
	}

	ove_r37.frequency_in_range = (ove_r37.frequency >= OVE_R37_FREQUENCY_RANGE_MIN_MHZ) &&
	                             (ove_r37.frequency <= OVE_R37_FREQUENCY_RANGE_MAX_MHZ);
}

// 5.7.4.2 Wiederzuschaltung
void ove_r37_check_reconnect_conditions(void) {
	// Only relevant while charging is blocked after a trip.
	if((ove_r37.state != OVE_R37_STATE_TRIPPED) && (ove_r37.state != OVE_R37_STATE_WAIT)) {
		return;
	}

	// Any violation of the supply window (re)starts the wait from scratch
	// (5.7.4.2 Prüfung 4c: reset of the wait time on renewed violation).
	if(!ove_r37_reconnect_supply_ok()) {
		ove_r37.state      = OVE_R37_STATE_TRIPPED;
		ove_r37.wait_start = 0;
		return;
	}

	// Supply is within the reconnect window: start the wait time tautom.
	if(ove_r37.state == OVE_R37_STATE_TRIPPED) {
		ove_r37.state      = OVE_R37_STATE_WAIT;
		ove_r37.wait_start = ove_r37_now_ms();
	}

	// Resume charging readiness once the wait time has elapsed.
	if(system_timer_is_time_elapsed_ms(ove_r37.wait_start, (uint32_t)ove_r37.reconnect_wait_s * 1000U)) {
		ove_r37.trip_reason = OVE_R37_TRIP_NONE;
		ove_r37.state       = OVE_R37_STATE_RAMP;
		ove_r37.ramp_start  = ove_r37_now_ms();
		ove_r37.ramp_limit  = OVE_R37_RAMP_START_MA;
	}
}

// 5.7.4.2 Hochlauframpe
void ove_r37_apply_power_ramp(void) {
	if(ove_r37.state != OVE_R37_STATE_RAMP) {
		return;
	}

	// Increase the offered current by 1 A every minute, starting at the
	// technical minimum, until the full current is reached.
	const uint32_t elapsed = system_timer_get_ms() - ove_r37.ramp_start;
	const uint32_t steps   = elapsed / OVE_R37_RAMP_STEP_MS;
	const uint32_t limit   = (uint32_t)OVE_R37_RAMP_START_MA + steps * OVE_R37_RAMP_STEP_MA;

	if(limit >= OVE_R37_RAMP_FULL_MA) {
		// Ramp finished: release the limit and return to normal operation.
		ove_r37.ramp_limit = OVE_R37_NO_LIMIT;
		ove_r37.state      = OVE_R37_STATE_NORMAL;
	} else {
		ove_r37.ramp_limit = (uint16_t)limit;
	}
}

// 5.9.3 Symmetriebedingungen
void ove_r37_check_phase_symmetry(void) {
	if(!ove_r37.current_valid) {
		ove_r37.symmetry_limit = OVE_R37_NO_LIMIT;
		return;
	}

	// Determine the maximum pairwise difference between the connected phase
	// currents (max - min over all connected phases).
	uint32_t i_max    = 0;
	uint32_t i_min    = 0xFFFFFFFFU;
	bool     any      = false;
	for(uint8_t i = 0; i < 3; i++) {
		if(ove_r37.phase_connected[i]) {
			any   = true;
			i_max = MAX(i_max, ove_r37.current[i]);
			i_min = MIN(i_min, ove_r37.current[i]);
		}
	}

	if(!any) {
		ove_r37.symmetry_limit = OVE_R37_NO_LIMIT;
		return;
	}

	const uint32_t diff = i_max - i_min;

	if(diff > OVE_R37_SYMMETRY_MAX_DIFF_MA) {
		// Cap the offered current to 16 A so the maximum phase difference can
		// not exceed 16 A. We have up to 60 s to react and act immediately.
		ove_r37.symmetry_limit = OVE_R37_SYMMETRY_MAX_DIFF_MA;
	} else if(diff < OVE_R37_SYMMETRY_RELEASE_DIFF_MA) {
		// Sufficiently balanced again: release the cap (hysteresis avoids
		// oscillation around the 16 A threshold).
		ove_r37.symmetry_limit = OVE_R37_NO_LIMIT;
	}
	// In the hysteresis band the previous symmetry_limit is kept.
}

// 5.9.2 Prüfung B
void ove_r37_apply_start_delay(void) {
	// No charge requested: clear the delay state.
	if(!ove_r37.charge_requested) {
		ove_r37.charge_requested_last = false;
		ove_r37.start_delay_ref       = 0;
		ove_r37.start_delay_active    = false;
		return;
	}

	// Rising edge of a charge request: take the delay reference time.
	if(!ove_r37.charge_requested_last) {
		ove_r37.charge_requested_last = true;
		ove_r37.start_delay_ref       = ove_r37_now_ms();
	}

	if(ove_r37.start_delay_s == 0) {
		ove_r37.start_delay_active = false;
		return;
	}

	// Hold the start off until the configured delay has elapsed.
	ove_r37.start_delay_active = !system_timer_is_time_elapsed_ms(ove_r37.start_delay_ref,
	                                                              (uint32_t)ove_r37.start_delay_s * 1000U);
}

void ove_r37_update_charging_slot(void) {
	uint16_t limit = OVE_R37_NO_LIMIT;

	switch(ove_r37.state) {
		case OVE_R37_STATE_DISABLED:
			limit = OVE_R37_NO_LIMIT;
			break;

		case OVE_R37_STATE_TRIPPED:
		case OVE_R37_STATE_WAIT:
			limit = 0;
			break;

		case OVE_R37_STATE_RAMP:
			limit = MIN(ove_r37.ramp_limit, ove_r37.symmetry_limit);
			break;

		case OVE_R37_STATE_NORMAL:
			limit = ove_r37.symmetry_limit;
			break;
	}

	// The configurable start delay holds off the charge start (5.9.2 B).
	if(((ove_r37.state == OVE_R37_STATE_NORMAL) || (ove_r37.state == OVE_R37_STATE_RAMP)) && ove_r37.start_delay_active) {
		limit = 0;
	}

	if(limit == OVE_R37_NO_LIMIT) {
		ove_r37.slot_active = false;
		ove_r37.max_current = 0;
	} else {
		ove_r37.slot_active = true;
		ove_r37.max_current = limit;
	}

	charging_slot.max_current[CHARGING_SLOT_OVE_R37] = ove_r37.max_current;
	charging_slot.active[CHARGING_SLOT_OVE_R37]      = ove_r37.slot_active;
}

uint8_t ove_r37_get_flags(void) {
	return (ove_r37.voltage_in_range   ? OVE_R37_FLAG_VOLTAGE_IN_RANGE   : 0) |
	       (ove_r37.frequency_in_range ? OVE_R37_FLAG_FREQUENCY_IN_RANGE : 0) |
	       (ove_r37.voltage_valid      ? OVE_R37_FLAG_VOLTAGE_VALID      : 0) |
	       (ove_r37.current_valid      ? OVE_R37_FLAG_CURRENT_VALID      : 0) |
	       (ove_r37.frequency_valid    ? OVE_R37_FLAG_FREQUENCY_VALID    : 0);
}
