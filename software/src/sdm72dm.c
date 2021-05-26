/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm72dm.c: SDM72DM driver
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

#include "sdm72dm.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/crc16.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

SDM72DM sdm72dm;

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

void sdm72dm_read_input_registers(uint8_t slave_address, uint16_t starting_address, uint16_t count) {
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

void sdm72dm_write_register(uint8_t slave_address, uint16_t starting_address, uint16_t payload) {
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

void sdm72dm_init(void) {
	memset(&sdm72dm, 0, sizeof(SDM72DM));
}

bool sdm72dm_get_read_input(uint32_t *data) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_READ_INPUT_REGISTERS) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	int8_t exception_code = 0;
	
	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		exception_code = MODBUS_EC_TIMEOUT;
	} else if(rs485.modbus_rtu.request.rx_frame[1] == rs485.modbus_rtu.request.tx_frame[1] + 0x80) {
		// Check if the slave response is an exception.
		exception_code = rs485.modbus_rtu.request.rx_frame[2];

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
		// we assume that a SDM72DM is attached to the EVSE.
		sdm72dm.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
		*data = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
	}

	return true; // increment state
}

bool sdm72dm_write_register_response(int8_t *exception_code) {
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

void sdm72dm_tick(void) {
	// TODO: Add additional external timeout for all states 
	//       with hardware unit reset in case of timeout
	switch(sdm72dm.state) {
		case 0: { // read power request
			sdm72dm_read_input_registers(1, 1281, 2);
			sdm72dm.state++;
			break;
		}

		case 1: { // read power
			if(sdm72dm_get_read_input(&sdm72dm.power.data)) {
				modbus_clear_request(&rs485);
				sdm72dm.state++;
			}
			break;
		}

		case 2: { // read relative energy request
			sdm72dm_read_input_registers(1, 389, 2);
			sdm72dm.state++;
			break;
		}

		case 3: { // read relative energy
			if(sdm72dm_get_read_input(&sdm72dm.energy_relative.data)) {
				modbus_clear_request(&rs485);
				sdm72dm.state++;
			}
			break;
		}

		case 4: { // read absolute energy request
			sdm72dm_read_input_registers(1, 73, 2);
			sdm72dm.state++;
			break;
		}

		case 5: { // read absolute energy
			if(sdm72dm_get_read_input(&sdm72dm.energy_absolute.data)) {
				modbus_clear_request(&rs485);
				sdm72dm.state++;
//				logd("power: %d, energy (rel): %d, energy (abs): %d\n\r", (int32_t)sdm72dm.power.f, (int32_t)sdm72dm.energy_relative.f, (int32_t)sdm72dm.energy_absolute.f);
			}
			break;
		}

		case 6: { // write reset
			if(sdm72dm.reset_energy_meter) {
				sdm72dm_write_register(1, 61457, 0x0003);
				sdm72dm.state++;
			} else {
				sdm72dm.state = 0;
			}
			break;
		}

		case 7: { // read write reset response
			int8_t exception_code;
			if(sdm72dm_write_register_response(&exception_code)) {
				if(exception_code == 0) {
					sdm72dm.reset_energy_meter = false;
				}
//				logd("reset response: %d\n\r", exception_code);
				modbus_clear_request(&rs485);
				sdm72dm.state++;
			}
			break;
		}
	}

	if(sdm72dm.state >= 8) {
		sdm72dm.state = 0;
	}
}
