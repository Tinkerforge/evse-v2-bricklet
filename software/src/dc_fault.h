/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * dc_fault.h: DC fault detection
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

#ifndef DC_FAULT_H
#define DC_FAULT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	DC_FAULT_NORMAL_CONDITION = 0,
	DC_FAULT_6MA_DC,
	DC_FAULT_SYSTEM,
	DC_FAULT_UNKOWN,
	DC_FAULT_CALIBRATION,
	DC_FAULT_20MA_AC,
	DC_FAULT_6MA_DC_AND_20MA_AC
} DCFaultState;

typedef enum {
	DC_FAULT_SENSOR_X904 = 0,
	DC_FAULT_SENSOR_X804 = 1,
} DCFaultSensorType;

typedef struct {
	DCFaultState state;
	DCFaultSensorType sensor_type;
	uint32_t last_fault_time;
	uint32_t last_run_time;

	bool x6;
	bool x30;
	bool error;

	bool     calibration_start;
	bool     calibration_start_external;
	bool     calibration_running;
	uint8_t  calibration_state;
	uint32_t calibration_time;
	bool     calibration_check[3];
} DCFault;

extern DCFault dc_fault;

void dc_fault_init(void);
void dc_fault_tick(void);
void dc_fault_calibration_reset(void);

#endif