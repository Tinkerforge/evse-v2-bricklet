/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm630.c: SDM630 driver
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

#include "sdm630.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/crc16.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

// Maps to registers in SDM630Register
const uint16_t sdm630_registers_to_read[] = {
	1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,47,49,53,57,61,63,67,71,73,75,77,79,81,83,85,87,101,103,105,201,203,205,207,225,235,237,239,241,243,245,249,251,259,261,263,265,267,269,335,337,339,341,343,345,347,349,351,353,355,357,359,361,363,365,367,369,371,373,375,377,379,381
};

const uint16_t sdm630_registers_fast_to_read[] = {
	53, 343, 7, 9, 11
};

SDM630 sdm630;
SDM630Register sdm630_register;
SDM630RegisterFast sdm630_register_fast;

#define SDM630_REGISTER_NUM      (sizeof(sdm630_registers_to_read)/sizeof(uint16_t))
#define SDM630_REGISTER_FAST_NUM (sizeof(sdm630_registers_fast_to_read)/sizeof(uint16_t))

static void modbus_store_tx_frame_data_bytes(const uint8_t *data, const uint16_t length) {
	for(uint16_t i = 0; i < length; i++) {
		ringbuffer_add(&rs485.ringbuffer_tx, data[i]);
	}
}

static void modbus_store_tx_frame_data_shorts(const uint16_t *data, const uint16_t length) {
	uint16_t u16_network_order = 0;
	uint8_t *_data = (uint8_t *)&u16_network_order;

	for(uint16_t i = 0; i < length; i++) {
		u16_network_order = HTONS(data[i]);

		ringbuffer_add(&rs485.ringbuffer_tx, _data[0]);
		ringbuffer_add(&rs485.ringbuffer_tx, _data[1]);
	}
}

static void modbus_add_tx_frame_checksum(void) {
	uint16_t checksum = crc16_modbus(rs485.ringbuffer_tx.buffer, ringbuffer_get_used(&rs485.ringbuffer_tx));

	ringbuffer_add(&rs485.ringbuffer_tx, checksum & 0xFF);
	ringbuffer_add(&rs485.ringbuffer_tx, checksum >> 8);
}

void sdm630_read_input_registers(uint8_t slave_address, uint16_t starting_address, uint16_t count) {
	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 10);

	uint8_t fc = MODBUS_FC_READ_INPUT_REGISTERS;

	// Fix endianness (LE->BE)
	starting_address = HTONS(starting_address-1);
	count = HTONS(count);

	// Constructing the frame in the TX buffer.
	modbus_store_tx_frame_data_bytes(&slave_address, 1); // Slave address.
	modbus_store_tx_frame_data_bytes(&fc, 1); // Function code.
	modbus_store_tx_frame_data_bytes((uint8_t *)&starting_address, 2);
	modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2);

	// Calculate checksum and put it at the end of the TX buffer.
	modbus_add_tx_frame_checksum();

	// Start master request timeout timing.
	rs485.modbus_rtu.request.time_ref_master_request_timeout = system_timer_get_ms();

	// Start TX.
	modbus_start_tx_from_buffer(&rs485);
}

void sdm630_write_register(uint8_t slave_address, uint16_t starting_address, uint16_t payload) {
	uint8_t fc = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
	uint16_t count = HTONS(1);
	uint8_t byte_count = 2;

	// Fix endianness (LE->BE).
	starting_address = HTONS(starting_address-1);

	// Constructing the frame in the TX buffer.
	modbus_store_tx_frame_data_bytes(&slave_address, 1); // Slave address.
	modbus_store_tx_frame_data_bytes(&fc, 1); // Function code.
	modbus_store_tx_frame_data_bytes((uint8_t *)&starting_address, 2); // Starting address.
	modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2); // Count.
	modbus_store_tx_frame_data_bytes((uint8_t *)&byte_count, 1); // Byte count.
	modbus_store_tx_frame_data_shorts(&payload, 1);
	modbus_add_tx_frame_checksum();

	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 10);
	modbus_start_tx_from_buffer(&rs485);

	// Start master request timeout timing.
	rs485.modbus_rtu.request.time_ref_master_request_timeout = system_timer_get_ms();
}

void sdm630_init(void) {
	uint32_t relative_energy_save = sdm630.relative_energy.data;
	memset(&sdm630, 0, sizeof(SDM630));
	memset(&sdm630_register, 0 , sizeof(SDM630Register));
	_Static_assert(SDM630_REGISTER_NUM == (sizeof(sdm630_register)/sizeof(uint32_t)), "Register size doesnt matcoh register numbers");
	_Static_assert(SDM630_REGISTER_FAST_NUM == (sizeof(sdm630_register_fast)/sizeof(uint32_t)), "Register Fast size doesnt matcoh register numbers");

	sdm630.relative_energy.data = relative_energy_save;
	sdm630.first_tick = system_timer_get_ms();
	sdm630.register_fast_time = system_timer_get_ms();
}

bool sdm630_get_read_input(uint32_t *data) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_READ_INPUT_REGISTERS) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		// Nothing
	} else if(rs485.modbus_rtu.request.rx_frame[1] == rs485.modbus_rtu.request.tx_frame[1] + 0x80) {
		// Check if the slave response is an exception.
		if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_FUNCTION) {
			rs485.modbus_common_error_counters.illegal_function++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_ADDRESS) {
			rs485.modbus_common_error_counters.illegal_data_address++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_VALUE) {
			rs485.modbus_common_error_counters.illegal_data_value++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_SLAVE_DEVICE_FAILURE) {
			rs485.modbus_common_error_counters.slave_device_failure++;
		}
	} else {
		// As soon as we were able to read our first package (including correct crc etc)
		// we assume that a SDM630 is attached to the EVSE.
		sdm630.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
		*data = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
	}

	return true; // increment state
}

bool sdm630_write_register_response(int8_t *exception_code) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
	   (rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
	   (rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_WRITE_MULTIPLE_REGISTERS) ||
	   !rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	*exception_code = 0;

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		*exception_code = MODBUS_EC_TIMEOUT;
	} else if(rs485.modbus_rtu.request.rx_frame[1] == rs485.modbus_rtu.request.tx_frame[1] + 0x80) {
		// Check if the slave response is an exception.
		*exception_code = rs485.modbus_rtu.request.rx_frame[2];

		if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_FUNCTION) {
			rs485.modbus_common_error_counters.illegal_function++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_ADDRESS) {
			rs485.modbus_common_error_counters.illegal_data_address++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_ILLEGAL_DATA_VALUE) {
			rs485.modbus_common_error_counters.illegal_data_value++;
		} else if(rs485.modbus_rtu.request.rx_frame[2] == MODBUS_EC_SLAVE_DEVICE_FAILURE) {
			rs485.modbus_common_error_counters.slave_device_failure++;
		}
	}
	
	return true; // increment state
}

void sdm630_tick(void) {
	static uint8_t last_state = 255;
	static bool read_fast = false;

	if(sdm630.first_tick != 0) {
		if(!system_timer_is_time_elapsed_ms(sdm630.first_tick, 2500)) {
			return;
		}
		sdm630.first_tick = 0;
	}

	if(last_state != sdm630.state) {
		sdm630.timeout = system_timer_get_ms();
		last_state = sdm630.state;
	} else {
		// If there is no state change at all for 60 seconds we assume that something is broken and trigger the watchdog.
		// This should never happen.
		if(system_timer_is_time_elapsed_ms(sdm630.timeout, 60000)) {
			while(true) {
				__NOP();
			}
		}
	}

	switch(sdm630.state) {
		case 0: { // request
			if(system_timer_is_time_elapsed_ms(sdm630.register_fast_time, 500) || (sdm630.register_fast_position > 0)) {
				sdm630_read_input_registers(1, sdm630_registers_fast_to_read[sdm630.register_fast_position], 2);
				read_fast = true;
			} else {
				sdm630_read_input_registers(1, sdm630_registers_to_read[sdm630.register_position], 2);
				read_fast = false;
			}
			sdm630.state++;
			break;
		}

		case 1: { // read
			bool ret = false;
			if(read_fast) {
				ret = sdm630_get_read_input(&(((uint32_t*)&sdm630_register_fast)[sdm630.register_fast_position]));
			} else {
				ret = sdm630_get_read_input(&(((uint32_t*)&sdm630_register)[sdm630.register_position]));
			}
			if(ret) {
				modbus_clear_request(&rs485);
				sdm630.state++;
				if(read_fast) {
					sdm630.register_fast_position++;
					if(sdm630.register_fast_position >= SDM630_REGISTER_FAST_NUM) {
						sdm630.register_fast_position = 0;
						sdm630.register_fast_time += 500;
						if(system_timer_is_time_elapsed_ms(sdm630.register_fast_time, 500)) {
							sdm630.register_fast_time = system_timer_get_ms();
						}
					}
				} else {
					sdm630.register_position++;
					if(sdm630.register_position >= SDM630_REGISTER_NUM) {
						sdm630.register_position = 0;
					}
				}
			}
			break;
		}
	}

	if(sdm630.state >= 2) {
		sdm630.state = 0;
	}
}
