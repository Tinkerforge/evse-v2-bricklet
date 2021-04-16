/* evse-v2-bricklet
 * Copyright (C) 2017 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * modbus.h: Modbus implementation (originally from rs485-bricklet)
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

#ifndef MODBUS_H
#define MODBUS_H

#include "rs485.h"

// Modbus function codes.
typedef enum {
	MODBUS_FC_READ_COILS = 1,
	MODBUS_FC_READ_HOLDING_REGISTERS = 3,
	MODBUS_FC_WRITE_SINGLE_COIL = 5,
	MODBUS_FC_WRITE_SINGLE_REGISTER = 6,
	MODBUS_FC_WRITE_MULTIPLE_COILS = 15,
	MODBUS_FC_WRITE_MULTIPLE_REGISTERS = 16,
	MODBUS_FC_READ_DISCRETE_INPUTS = 2,
	MODBUS_FC_READ_INPUT_REGISTERS = 4,
} ModbusFunctionCode;

// Modbus exception codes.
typedef enum {
	MODBUS_EC_TIMEOUT = -1,
	MODBUS_EC_ILLEGAL_FUNCTION = 1,
	MODBUS_EC_ILLEGAL_DATA_ADDRESS = 2,
	MODBUS_EC_ILLEGAL_DATA_VALUE = 3,
	MODBUS_EC_SLAVE_DEVICE_FAILURE = 4,
} ModbusExceptionCode;

typedef struct {
	uint8_t address;
	uint8_t function_code;
	uint8_t exception_code;
	uint16_t checksum;
} __attribute__((__packed__)) ModbusExceptionResponse;

void modbus_init(RS485 *rs485);
void modbus_clear_request(RS485 *rs485);
bool modbus_slave_check_address(RS485 *rs485);
void modbus_start_tx_from_buffer(RS485 *rs485);
bool modbus_check_frame_checksum(RS485 *rs485);
bool modbus_master_check_slave_response(RS485 *rs485);
void modbus_update_rtu_wire_state_machine(RS485 *rs485);
bool modbus_slave_check_function_code_imlemented(RS485 * rs485);
void modbus_init_new_request(RS485 *rs485, RS485ModbusRequestState state, uint16_t length);
void modbus_report_exception(RS485 *rs485, uint8_t function_code, ModbusExceptionCode exception_code);

#endif
