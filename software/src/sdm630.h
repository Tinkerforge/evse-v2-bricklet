/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm630.h: SDM630 driver
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

#ifndef SDM630_H
#define SDM630_H

#include <stdint.h>
#include <stdbool.h>

#define SDM630_PHASE_NUM 3

#define SDM630_HOLDING_REG_SYSTEM_TYPE 11 // 40011
#define SDM630_HOLDING_REG_PASSWORD    25 // 40025

#define SDM630_SYSTEM_TYPE_1P2W        1.0f
#define SDM630_SYSTEM_TYPE_3P43        2.0f
#define SDM630_SYSTEM_TYPE_3P4W        3.0f

#define SDM630_PASSWORD                1000.0f

typedef union {
	float f;
	uint32_t data;
} SDM630RegisterType;

typedef struct {
	SDM630RegisterType line_to_neutral_volts[SDM630_PHASE_NUM];
	SDM630RegisterType current[SDM630_PHASE_NUM];
	SDM630RegisterType power[SDM630_PHASE_NUM];
	SDM630RegisterType volt_amps[SDM630_PHASE_NUM];
	SDM630RegisterType volt_amps_reactive[SDM630_PHASE_NUM];
	SDM630RegisterType power_factor[SDM630_PHASE_NUM];
	SDM630RegisterType phase_angle[SDM630_PHASE_NUM];
	SDM630RegisterType average_line_to_neutral_volts;
	SDM630RegisterType average_line_current;
	SDM630RegisterType sum_of_line_currents;
	SDM630RegisterType total_system_power;
	SDM630RegisterType total_system_volt_amps;
	SDM630RegisterType total_system_var;
	SDM630RegisterType total_system_power_factor;
	SDM630RegisterType total_system_phase_angle;
	SDM630RegisterType frequency_of_supply_voltages;
	SDM630RegisterType total_import_kwh;
	SDM630RegisterType total_export_kwh;
	SDM630RegisterType total_import_kvarh;
	SDM630RegisterType total_export_kvarh;
	SDM630RegisterType total_vah;
	SDM630RegisterType ah;
	SDM630RegisterType total_system_power_demand;
	SDM630RegisterType maximum_total_system_power_demand;
	SDM630RegisterType total_system_va_demand;
	SDM630RegisterType maximum_total_system_va_demand;
	SDM630RegisterType neutral_current_demand;
	SDM630RegisterType maximum_neutral_current_demand;
	SDM630RegisterType line1_to_line2_volts;
	SDM630RegisterType line2_to_line3_volts;
	SDM630RegisterType line3_to_line1_volts;
	SDM630RegisterType average_line_to_line_volts;
	SDM630RegisterType neutral_current;
	SDM630RegisterType ln_volts_thd[SDM630_PHASE_NUM];
	SDM630RegisterType current_thd[SDM630_PHASE_NUM];
	SDM630RegisterType average_line_to_neutral_volts_thd;
	SDM630RegisterType current_demand[SDM630_PHASE_NUM];
	SDM630RegisterType maximum_current_demand[SDM630_PHASE_NUM];
	SDM630RegisterType line1_to_line2_volts_thd;
	SDM630RegisterType line2_to_line3_volts_thd;
	SDM630RegisterType line3_to_line1_volts_thd;
	SDM630RegisterType average_line_to_line_volts_thd;
	SDM630RegisterType total_kwh_sum;
	SDM630RegisterType total_kvarh_sum;
	SDM630RegisterType import_kwh[SDM630_PHASE_NUM];
	SDM630RegisterType export_kwh[SDM630_PHASE_NUM];
	SDM630RegisterType total_kwh[SDM630_PHASE_NUM];
	SDM630RegisterType import_kvarh[SDM630_PHASE_NUM];
	SDM630RegisterType export_kvarh[SDM630_PHASE_NUM];
	SDM630RegisterType total_kvarh[SDM630_PHASE_NUM];
} __attribute__((__packed__)) SDM630Register;

typedef struct {
	SDM630RegisterType power;
	SDM630RegisterType absolute_energy;
	SDM630RegisterType current_per_phase[SDM630_PHASE_NUM];
} __attribute__((__packed__)) SDM630RegisterFast;

typedef struct {
	uint8_t state;
	uint16_t register_position;
	uint16_t register_fast_position;

	uint32_t register_fast_time;

	bool available;
	bool reset_energy_meter;

	uint32_t timeout;
	uint32_t first_tick;

	SDM630RegisterType system_type_write;
	SDM630RegisterType system_type_read;
	bool new_system_type;

	bool phases_connected[3];

	SDM630RegisterType relative_energy;
} SDM630;

extern SDM630 sdm630;
extern SDM630Register sdm630_register;
extern SDM630RegisterFast sdm630_register_fast;

void sdm630_init(void);
void sdm630_tick(void);

#endif