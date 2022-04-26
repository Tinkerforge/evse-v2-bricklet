/* evse-v2-bricklet
 * Copyright (C) 2021-2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm.h: SDM630 and SDM72DM-V2 driver
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

#ifndef SDM_H
#define SDM_H

#include <stdint.h>
#include <stdbool.h>

#define SDM_PHASE_NUM 3

#define SDM_HOLDING_REG_SYSTEM_TYPE 11    // 40011
#define SDM_HOLDING_REG_SYSTEM_KPPA 15    // 40015
#define SDM_HOLDING_REG_PASSWORD    25    // 40025
#define SDM_HOLDING_REG_METER_CODE  64515 // 464515

#define SDM_SYSTEM_TYPE_1P2W        1.0f
#define SDM_SYSTEM_TYPE_3P43        2.0f
#define SDM_SYSTEM_TYPE_3P4W        3.0f

#define SDM_PASSWORD                1000.0f

typedef union {
	float f;
	uint32_t data;
	uint16_t u16[2];
} SDMRegisterType;

typedef struct {
	SDMRegisterType line_to_neutral_volts[SDM_PHASE_NUM];
	SDMRegisterType current[SDM_PHASE_NUM];
	SDMRegisterType power[SDM_PHASE_NUM];
	SDMRegisterType volt_amps[SDM_PHASE_NUM];
	SDMRegisterType volt_amps_reactive[SDM_PHASE_NUM];
	SDMRegisterType power_factor[SDM_PHASE_NUM];
	SDMRegisterType phase_angle[SDM_PHASE_NUM];
	SDMRegisterType average_line_to_neutral_volts;
	SDMRegisterType average_line_current;
	SDMRegisterType sum_of_line_currents;
	SDMRegisterType total_system_power;
	SDMRegisterType total_system_volt_amps;
	SDMRegisterType total_system_var;
	SDMRegisterType total_system_power_factor;
	SDMRegisterType total_system_phase_angle;
	SDMRegisterType frequency_of_supply_voltages;
	SDMRegisterType total_import_kwh;
	SDMRegisterType total_export_kwh;
	SDMRegisterType total_import_kvarh;
	SDMRegisterType total_export_kvarh;
	SDMRegisterType total_vah;
	SDMRegisterType ah;
	SDMRegisterType total_system_power_demand;
	SDMRegisterType maximum_total_system_power_demand;
	SDMRegisterType total_system_va_demand;
	SDMRegisterType maximum_total_system_va_demand;
	SDMRegisterType neutral_current_demand;
	SDMRegisterType maximum_neutral_current_demand;
	SDMRegisterType line1_to_line2_volts;
	SDMRegisterType line2_to_line3_volts;
	SDMRegisterType line3_to_line1_volts;
	SDMRegisterType average_line_to_line_volts;
	SDMRegisterType neutral_current;
	SDMRegisterType ln_volts_thd[SDM_PHASE_NUM];
	SDMRegisterType current_thd[SDM_PHASE_NUM];
	SDMRegisterType average_line_to_neutral_volts_thd;
	SDMRegisterType average_line_to_current_thd;
	SDMRegisterType current_demand[SDM_PHASE_NUM];
	SDMRegisterType maximum_current_demand[SDM_PHASE_NUM];
	SDMRegisterType line1_to_line2_volts_thd;
	SDMRegisterType line2_to_line3_volts_thd;
	SDMRegisterType line3_to_line1_volts_thd;
	SDMRegisterType average_line_to_line_volts_thd;
	SDMRegisterType total_kwh_sum;
	SDMRegisterType total_kvarh_sum;
	SDMRegisterType import_kwh[SDM_PHASE_NUM];
	SDMRegisterType export_kwh[SDM_PHASE_NUM];
	SDMRegisterType total_kwh[SDM_PHASE_NUM];
	SDMRegisterType import_kvarh[SDM_PHASE_NUM];
	SDMRegisterType export_kvarh[SDM_PHASE_NUM];
	SDMRegisterType total_kvarh[SDM_PHASE_NUM];
} __attribute__((__packed__)) SDMRegister;

typedef struct {
	SDMRegisterType power;
	SDMRegisterType absolute_energy;
	SDMRegisterType current_per_phase[SDM_PHASE_NUM];
} __attribute__((__packed__)) SDMRegisterFast;

typedef enum {
	SDM_METER_TYPE_UNKNOWN = 0,
	SDM_METER_TYPE_SDM630 = 1,
	SDM_METER_TYPE_SDM72V2 = 2,
} SDMMeterType;

typedef struct {
	uint8_t state;
	uint16_t register_position;
	uint16_t register_fast_position;

	uint32_t register_fast_time;

	bool available;
	bool reset_energy_meter;

	uint32_t timeout;
	uint32_t first_tick;
	uint32_t error_wait_time;

	SDMRegisterType system_type_write;
	SDMRegisterType system_type_read;
	bool new_system_type;

	bool phases_connected[3];

	SDMRegisterType relative_energy;
	SDMMeterType meter_type;
} SDM;

extern SDM sdm;
extern SDMRegister sdm_register;
extern SDMRegisterFast sdm_register_fast;

void sdm_init(void);
void sdm_tick(void);

#endif