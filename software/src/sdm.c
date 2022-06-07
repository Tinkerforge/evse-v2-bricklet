/* evse-v2-bricklet
 * Copyright (C) 2021-2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm.c: SDM630 and SDM72DM-V2 driver
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

#include "sdm.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/crc16.h"
#include "bricklib2/logging/logging.h"

#include "rs485.h"
#include "modbus.h"

// Superset of a SDM meter register map
// Currently this consists of the SDM630 registers.
const uint16_t sdm_registers_to_read[] = {
	1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43,47,49,53,57,61,63,67,71,73,75,77,79,81,83,85,87,101,103,105,107,201,203,205,207,225,235,237,239,241,243,245,249,251,259,261,263,265,267,269,335,337,339,341,343,345,347,349,351,353,355,357,359,361,363,365,367,369,371,373,375,377,379,381
};

// Registers of the superset that are available in SDM72V2
const bool sdm_registers_available_in_sdm72v2[] = {
	1,1,1,1,1,1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// Registers read with high frequency
const uint16_t sdm_registers_fast_to_read[] = {
	53, 343, 7, 9, 11
};

SDM sdm;
SDMRegister sdm_register;
SDMRegisterFast sdm_register_fast;

#define SDM_REGISTER_NUM      (sizeof(sdm_registers_to_read)/sizeof(uint16_t))
#define SDM_REGISTER_FAST_NUM (sizeof(sdm_registers_fast_to_read)/sizeof(uint16_t))

static void modbus_store_tx_frame_data_bytes(const uint8_t *data, const uint16_t length) {
	for(uint16_t i = 0; i < length; i++) {
		ringbuffer_add(&rs485.ringbuffer_tx, data[i]);
	}
}

static void modbus_store_tx_frame_data_shorts(const uint16_t *data, const uint16_t length) {
	for(uint16_t i = 0; i < length; i++) {
		ringbuffer_add(&rs485.ringbuffer_tx, data[i] >> 8);
		ringbuffer_add(&rs485.ringbuffer_tx, data[i] & 0xFF);
	}
}

static void modbus_add_tx_frame_checksum(void) {
	uint16_t checksum = crc16_modbus(rs485.ringbuffer_tx.buffer, ringbuffer_get_used(&rs485.ringbuffer_tx));

	ringbuffer_add(&rs485.ringbuffer_tx, checksum & 0xFF);
	ringbuffer_add(&rs485.ringbuffer_tx, checksum >> 8);
}

void sdm_read_input_registers(uint8_t slave_address, uint16_t starting_address, uint16_t count) {
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

void sdm_read_holding_registers(uint8_t slave_address, uint16_t starting_address, uint16_t count) {
	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 10);

	uint8_t fc = MODBUS_FC_READ_HOLDING_REGISTERS;

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

void sdm_write_register(uint8_t slave_address, uint16_t starting_address, SDMRegisterType *payload) {
	uint8_t fc = MODBUS_FC_WRITE_MULTIPLE_REGISTERS;
	uint16_t count = HTONS(2);
	uint8_t byte_count = 4;

	// Fix endianness (LE->BE).
	starting_address = HTONS(starting_address-1);

	// Constructing the frame in the TX buffer.
	modbus_store_tx_frame_data_bytes(&slave_address, 1); // Slave address.
	modbus_store_tx_frame_data_bytes(&fc, 1); // Function code.
	modbus_store_tx_frame_data_bytes((uint8_t *)&starting_address, 2); // Starting address.
	modbus_store_tx_frame_data_bytes((uint8_t *)&count, 2); // Count.
	modbus_store_tx_frame_data_bytes((uint8_t *)&byte_count, 1); // Byte count.
	modbus_store_tx_frame_data_shorts(&payload->u16[1], 1);
	modbus_store_tx_frame_data_shorts(&payload->u16[0], 1);
	modbus_add_tx_frame_checksum();

	modbus_init_new_request(&rs485, MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE, 13);

	// Start master request timeout timing.
	rs485.modbus_rtu.request.time_ref_master_request_timeout = system_timer_get_ms();

	modbus_start_tx_from_buffer(&rs485);

}

void sdm_init(void) {
	uint32_t relative_energy_save = sdm.relative_energy.data;
	memset(&sdm, 0, sizeof(SDM));
	memset(&sdm_register, 0 , sizeof(SDMRegister));
	_Static_assert(SDM_REGISTER_NUM == (sizeof(sdm_register)/sizeof(uint32_t)), "Register size doesnt matcoh register numbers");
	_Static_assert(SDM_REGISTER_FAST_NUM == (sizeof(sdm_register_fast)/sizeof(uint32_t)), "Register Fast size doesnt matcoh register numbers");

	sdm.relative_energy.data = relative_energy_save;
	sdm.first_tick = system_timer_get_ms();
	sdm.register_fast_time = system_timer_get_ms();
}

bool sdm_get_read_input(uint32_t *data) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_READ_INPUT_REGISTERS) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		sdm.error_wait_time = system_timer_get_ms();
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
		// we assume that a SDM is attached to the EVSE.
		sdm.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
		*data = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
	}

	return true; // increment state
}

bool sdm_get_holding_input_16bit(uint16_t *data) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_READ_HOLDING_REGISTERS) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		sdm.error_wait_time = system_timer_get_ms();
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
		// we assume that a SDM is attached to the EVSE.
		sdm.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
		*data = (d[0] << 8) | (d[1] << 0);
	}

	return true; // increment state
}

bool sdm_get_holding_input_32bit(uint32_t *data) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
		(rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
		(rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_READ_HOLDING_REGISTERS) ||
		!rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		sdm.error_wait_time = system_timer_get_ms();
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
		// we assume that a SDM is attached to the EVSE.
		sdm.available = true;

		uint8_t *d = &rs485.modbus_rtu.request.rx_frame[3];
		*data = (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
	}

	return true; // increment state
}

bool sdm_write_register_response(void) {
	if((rs485.mode != MODE_MODBUS_MASTER_RTU) ||
	   (rs485.modbus_rtu.request.state != MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) ||
	   (rs485.modbus_rtu.request.tx_frame[1] != MODBUS_FC_WRITE_MULTIPLE_REGISTERS) ||
	   !rs485.modbus_rtu.request.cb_invoke) {
		return false; // don't increment state
	}

	// Check if the request has timed out.
	if(rs485.modbus_rtu.request.master_request_timed_out) {
		sdm.error_wait_time = system_timer_get_ms();
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
	}
	
	return true; // increment state
}

void sdm_handle_new_system_type(void) {
	// Update phases connected bool array (this is also used in communication.c)
	sdm.phases_connected[0] = sdm_register.line_to_neutral_volts[0].f > 180.0f;
	sdm.phases_connected[1] = sdm_register.line_to_neutral_volts[1].f > 180.0f;
	sdm.phases_connected[2] = sdm_register.line_to_neutral_volts[2].f > 180.0f;

	if(sdm.system_type_read.f == SDM_SYSTEM_TYPE_3P4W) {
		// Change system type if 3-phase is configured, but 1-phase is connected
		if(sdm.phases_connected[0] && !sdm.phases_connected[1] && !sdm.phases_connected[2]) {
			sdm.new_system_type = true;
		}
	} else if(sdm.system_type_read.f == SDM_SYSTEM_TYPE_1P2W) {
		// Change system type if 1-phase is configured, but 3-phase is connected
		if(sdm.phases_connected[0] && sdm.phases_connected[1] && sdm.phases_connected[2]) {
			sdm.new_system_type = true;
		}
	} else {
		// Change system type if un-allowed system type is configured 
		sdm.new_system_type = true;
	}
}

void sdm_tick(void) {
	static uint8_t last_state = 255;
	static bool read_fast = false;


	if(sdm.error_wait_time != 0) {
		if(!system_timer_is_time_elapsed_ms(sdm.error_wait_time, 500)) {
			return;
		}
		sdm.error_wait_time = 0;
	}

	if(sdm.first_tick != 0) {
		if(!system_timer_is_time_elapsed_ms(sdm.first_tick, 2500)) {
			return;
		}
		sdm.first_tick = 0;
	}

	if(last_state != sdm.state) {
		sdm.timeout = system_timer_get_ms();
		last_state = sdm.state;
	} else {
		// If there is no state change at all for 60 seconds we assume that something is broken and trigger the watchdog.
		// This should never happen.
		if(system_timer_is_time_elapsed_ms(sdm.timeout, 60000)) {
			while(true) {
				__NOP();
			}
		}
	}

	switch(sdm.state) {
		case 0: { // request
			// The SDM72V2 does not have all of the registeres that SDM has, we ignore the once that are not available.
			if(sdm.meter_type == SDM_METER_TYPE_SDM72V2) {
				while(!sdm_registers_available_in_sdm72v2[sdm.register_position]) {
					sdm.register_position++;
					if(sdm.register_position >= SDM_REGISTER_NUM) {
						sdm.register_position = 0;
					}
				}
			}
			if(sdm.meter_type == SDM_METER_TYPE_UNKNOWN) {
				sdm_read_holding_registers(1, SDM_HOLDING_REG_METER_CODE, 1);
				sdm.state = 100;
				break;
			} else if(system_timer_is_time_elapsed_ms(sdm.register_fast_time, 500) || (sdm.register_fast_position > 0)) {
				sdm_read_input_registers(1, sdm_registers_fast_to_read[sdm.register_fast_position], 2);
				read_fast = true;
			} else {
				sdm_read_input_registers(1, sdm_registers_to_read[sdm.register_position], 2);
				read_fast = false;
			}
			sdm.state++;
			break;
		}

		case 1: { // read
			bool ret = false;
			if(read_fast) {
				ret = sdm_get_read_input(&(((uint32_t*)&sdm_register_fast)[sdm.register_fast_position]));
			} else {
				ret = sdm_get_read_input(&(((uint32_t*)&sdm_register)[sdm.register_position]));
			}
			if(ret) {
				modbus_clear_request(&rs485);
				sdm.state++;
				if(read_fast) {
					sdm.register_fast_position++;
					if(sdm.register_fast_position >= SDM_REGISTER_FAST_NUM) {
						sdm.register_fast_position = 0;
						sdm.register_fast_time += 500;
						if(system_timer_is_time_elapsed_ms(sdm.register_fast_time, 500)) {
							sdm.register_fast_time = system_timer_get_ms();
						}
					}
				} else {
					sdm.register_position++;
					if(sdm.register_position >= SDM_REGISTER_NUM) {
						sdm.register_position = 0;
					}
				}
			}
			break;
		}

		case 2: { // request system type from holding register
			if(sdm.register_position != 0) {
				sdm.state = 0;
				break;
			}
			sdm_read_holding_registers(1, SDM_HOLDING_REG_SYSTEM_TYPE, 2);
			sdm.state++;
			break;
		}

		case 3: { // read system type from holding register
			bool ret = sdm_get_holding_input_32bit((uint32_t*)&sdm.system_type_read);
			if(ret) {
				sdm_handle_new_system_type();
				modbus_clear_request(&rs485);
				sdm.state++;
			}
			break;
		}

		case 4: { // write password for phase change (if new phase configuration)
			if(!sdm.new_system_type) {
				sdm.state = 0;
				break;
			}

			//rs485.modbus_common_error_counters.slave_device_failure = 4;
			SDMRegisterType password;
			password.f = SDM_PASSWORD;
			modbus_clear_request(&rs485);
			if(sdm.meter_type == SDM_METER_TYPE_SDM72V2) {
				// For SDM72V2 the password has to be written to KPPA register
				sdm_write_register(1, SDM_HOLDING_REG_SYSTEM_KPPA, &password);
			} else {
				sdm_write_register(1, SDM_HOLDING_REG_PASSWORD, &password);
			}
			sdm.state++;
			break;
		}

		case 5: { // check write password response
			bool ret = sdm_write_register_response();
			if(ret) {
				modbus_clear_request(&rs485);
				sdm.state++;
			}
			break;
		}

		case 6: { // write new system type
			SDMRegisterType system_type;
			if(sdm.phases_connected[0] && !sdm.phases_connected[1] && !sdm.phases_connected[2]) {
				system_type.f = SDM_SYSTEM_TYPE_1P2W;
			} else if(sdm.phases_connected[0] && sdm.phases_connected[1] && sdm.phases_connected[2]) {
				system_type.f = SDM_SYSTEM_TYPE_3P4W;
			} else {
				sdm.state = 0;
				break;
			}

			modbus_clear_request(&rs485);
			sdm_write_register(1, SDM_HOLDING_REG_SYSTEM_TYPE, &system_type);
			sdm.state++;
			break;
		}

		case 7: { // check system type write response
			// This returns illegal function on SDM72V2
			bool ret = sdm_write_register_response();
			if(ret) {
				modbus_clear_request(&rs485);
				sdm.state++;
			}
			break;
		}

		case 100: { // check meter type
			uint16_t meter_code = 0;
			bool ret = sdm_get_holding_input_16bit(&meter_code);
			if(ret) {
				switch(meter_code) {
					case 0x0084: sdm.meter_type = SDM_METER_TYPE_UNKNOWN; break;  // 0x0084 is SDM72V1 (not supported)
					case 0x0089: sdm.meter_type = SDM_METER_TYPE_SDM72V2; break;  // Compare datasheet page 16 meter code
					case 0x0070: sdm.meter_type = SDM_METER_TYPE_SDM630;  break;

					// Some early versions of the SDM630 don't have the meter type register.
					// Because of this we default to SDM630.
					default:     sdm.meter_type = SDM_METER_TYPE_SDM630;  break;
				}

				modbus_clear_request(&rs485);
				sdm.state++;
			}
			break;
		}

		default: {
			sdm.state = 0;
		}
	}
}
