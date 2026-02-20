/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * eichrecht.c: Eichrecht implementation
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

#include "eichrecht.h"

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/warp/meter.h"
#include "bricklib2/warp/meter_iskra.h"
#include "bricklib2/warp/modbus.h"
#include "bricklib2/hal/system_timer/system_timer.h"

static const char *if_strings[] = {
    "RFID_NONE", "RFID_PLAIN", "RFID_RELATED", "RFID_PSK", "OCPP_NONE", "OCPP_RS", "OCPP_AUTH", "OCPP_RS_TLS", "OCPP_AUTH_TLS", "OCPP_CACHE", "OCPP_WHITELIST", "OCPP_CERTIFIED", "ISO15118_NONE", "ISO15118_PNC", "PLMN_NONE", "PLMN_RING", "PLMN_SMS"
};

static const char *it_strings[] = {
    "NONE", "DENIED", "UNDEFINED", "ISO14443", "ISO15693", "EMAID", "EVCCID", "EVCOID", "ISO7812", "CARD_TXN_NR", "CENTRAL", "CENTRAL_1", "CENTRAL_2", "LOCAL", "LOCAL_1", "LOCAL_2", "PHONE_NUMBER", "KEY_CODE"
};

static const char *ct_strings[] = {
    "EVSEID", "CBIDC"
};

Eichrecht eichrecht;

void eichrecht_init(void) {
    memset(&eichrecht, 0, sizeof(Eichrecht));

    // Eichrecht support only on v4 hardware
    if(!hardware_version.is_v4) {
        return;
    }
}

const char *eichrecht_get_if_string(uint8_t index) {
    if (index < ARRAY_SIZE(if_strings)) {
        return if_strings[index];
    } else {
        return NULL;
    }
}

const char *eichrecht_get_it_string(void) {
    if (eichrecht.ocmf.it < ARRAY_SIZE(it_strings)) {
        return it_strings[eichrecht.ocmf.it];
    } else {
        return "";
    }
}

const char *eichrecht_get_ct_string(void) {
    if (eichrecht.ocmf.ct < ARRAY_SIZE(ct_strings)) {
        return ct_strings[eichrecht.ocmf.ct];
    } else {
        return "";
    }
}

static char *itoa_p(uint8_t value, char *str) {
	if(value >= 100) {
		str[2] = '0' + (value % 10);
		str[1] = '0' + ((value / 10) % 10);
		str[0] = '0' + (value / 100);
		return str + 3;
	}

	if(value >= 10) {
		str[1] = '0' + (value % 10);
		str[0] = '0' + (value / 10);
		return str + 2;
	}

	str[0] = '0' + value;
	return str + 1;
}

void eichrecht_create_dataset(void) {
    // WM3M4C manual:
    // JSON names must be in specified order and without whitespaces. Downloaded message should look like
    // {"FV":"1.0","GI":"","GS":"","PG":"","MV":"","MM":"","MS":"","MF":"","IS":true,"IF":[],"IT":"NONE","ID":"","CT":"EVSEID","CI":"","RD":[]}
    // Example of valid JSON dataset (newlines are added for better readability):
    // "FV":"1.0",
    // "GI":"Gateway 1",
    // "GS":"123456789",
    // "PG":"",
    // "MV":"",
    // "MM":"",
    // "MS":"",
    // "MF": "",
    // "IS":true,
    // "IF":[
    // "RFID_PLAIN",
    // "OCPP_RS_TLS"
    // ],
    // "IT":"ISO14443",
    // "ID":"1F2D3A4F5506C7",
    // "CT":"EVSEID",
    // "CI":"",
    // "RD":[]
    // }

    memset(eichrecht.dataset_in, 0, sizeof(eichrecht.dataset_in));

    // The absolute maximum size of the dataset here is known (since all possible strings are predefined).
    // The dataset size was chosen big enough to hold the maximum possible size.
    // Thus we can use unsafe string functions here.
    char *ptr = eichrecht.dataset_in;

    ptr = stpcpy(ptr, "{\"FV\":\"1.3\",\"GI\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.gi);
    ptr = stpcpy(ptr, "\",\"GS\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.gs);
    ptr = stpcpy(ptr, "\",\"PG\":\"\",\"MV\":\"\",\"MM\":\"\",\"MS\":\"\",\"MF\":\"\",\"IS\":");
    ptr = stpcpy(ptr, eichrecht.ocmf.is ? "true" : "false");
    ptr = stpcpy(ptr, ",\"IF\":[");

    bool first = true;
    for (int i = 0; i < 4; i++) {
        const char *if_string = eichrecht_get_if_string(eichrecht.ocmf.if_[i]);
        if (if_string) {
            if (!first) {
                *ptr++ = ',';
            }
            first = false;
            *ptr++ = '"';
            ptr = stpcpy(ptr, if_string);
            *ptr++ = '"';
        }
    }

    ptr = stpcpy(ptr, "],\"IT\":\"");
    ptr = stpcpy(ptr, eichrecht_get_it_string());
    ptr = stpcpy(ptr, "\",\"ID\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.id);
    ptr = stpcpy(ptr, "\",\"CF\":\"");

    uint32_t fw = BOOTLOADER_FIRMWARE_CONFIGURATION_POINTER->firmware_version;
    ptr = itoa_p((fw >> 16) & 0xFF, ptr);
    *ptr++ = '.';
    ptr = itoa_p((fw >> 8)  & 0xFF, ptr);
    *ptr++ = '.';
    ptr = itoa_p((fw >> 0)  & 0xFF, ptr);

    ptr = stpcpy(ptr, "\",\"CT\":\"");
    ptr = stpcpy(ptr, eichrecht_get_ct_string());
    ptr = stpcpy(ptr, "\",\"CI\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.ci);
    ptr = stpcpy(ptr, "\",\"RD\":[]}");
}


void eichrecht_tick(void) {
    // Eichrecht support only on v4 hardware
    if(!hardware_version.is_v4) {
        return;
    }

    if(eichrecht.new_transaction) {
        eichrecht_create_dataset();
        eichrecht.new_transaction = false;
        eichrecht.transaction_state = 1;
    }
}

bool eichrecht_iskra_write_utc_time_offset(const int16_t utc_time_offset) {
    switch(eichrecht.transaction_inner_state) {
        case 0: { // Write utc time offset
            MeterRegisterType payload;
            payload.i16_single = utc_time_offset;

            meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, 7053+1, &payload);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // Check utc time offset write response
            bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}

bool eichrecht_iskra_write_unix_time(const uint32_t unix_time) {
    switch(eichrecht.transaction_inner_state) {
        case 0: { // Write unix time
            MeterRegisterType payload;
            payload.u32 = unix_time;

            meter_write_register(MODBUS_FC_WRITE_MULTIPLE_REGISTERS, meter.slave_address, 7054+1, &payload);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // Check unix time write response
            bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}


bool eichrecht_iskra_write_signature_format(uint16_t signature_format) {
    switch(eichrecht.transaction_inner_state) {
        case 0: { // Write unix time
            MeterRegisterType payload;
            payload.u16_single = signature_format;

            meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, 7059+1, &payload);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // Check unix time write response
            bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}


bool eichrecht_iskra_write_dataset(void) {
    switch(eichrecht.transaction_inner_state) {
        case 0: {
            eichrecht.dataset_in_index = 0;
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // Write dataset
            uint16_t length = MIN(240, strlen(eichrecht.dataset_in) - eichrecht.dataset_in_index);
            if((length & 1) == 1) {
                length++; // Make even
            }
            meter_write_string(meter.slave_address, 7100+1 + (eichrecht.dataset_in_index / 2), &eichrecht.dataset_in[eichrecht.dataset_in_index], length);

            eichrecht.transaction_inner_state++;
            eichrecht.dataset_in_index += length;
            return false;
        }

        case 2: { // Check dataset write response
			bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS);
			if(ret) {
				modbus_clear_request(&rs485);
                if(eichrecht.dataset_in_index < strlen(eichrecht.dataset_in)) {
                    // More to write
                    eichrecht.transaction_inner_state = 1;
                } else {
                    // Done
                    eichrecht.transaction_inner_state++;
                }
			}
            return false;
        }

        case 3: { // Write dataset length
            MeterRegisterType payload;
            payload.u16_single = strlen(eichrecht.dataset_in);
            //payload.u16_single = 4;

            meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, 7056+1, &payload);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 4: { // Check dataset length write response
            bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}

bool eichrecht_iskra_send_transaction_command(const char command) {
    switch(eichrecht.transaction_inner_state) {
        case 0: { // Send transaction command
            MeterRegisterType payload;
            payload.u16_single = command << 8;

            meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, 7051+1, &payload);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // Check transaction command write response
            bool ret = meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}

bool eichrecht_iskra_get_measurement_status(uint16_t *status) {
    switch(eichrecht.transaction_inner_state) {
		case 0: { // request measurement status from holding register
            *status = 0xFFFF;
			meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7000+1, 1);
			eichrecht.transaction_inner_state++;
			return false;
		}

		case 1: { // read measurement status from holding register
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, status, 1);
			if(ret) {
				modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
			}
			return false;
		}
    }

    return false;
}

bool eichrecht_iskra_get_signature_status(uint16_t *status) {
    switch(eichrecht.transaction_inner_state) {
		case 0: { // request signature status from holding register
            *status = 0xFFFF;
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7052+1, 1);
			eichrecht.transaction_inner_state++;
			return false;
		}

		case 1: { // read signature status from holding register
			bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, status, 1);
			if(ret) {
				modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
			}
			return false;
		}
    }

    return false;
}

bool eichrecht_iskra_read_dataset(void) {
    switch(eichrecht.transaction_inner_state) {
        case 0: {
            memset(eichrecht.dataset_out, 0, sizeof(eichrecht.dataset_out));
            eichrecht.dataset_out_index = 0;
            eichrecht.dataset_out_length = 0;
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // request dataset length from holding register
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7057+1, 1);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 2: { // read dataset length from holding register
            bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &eichrecht.dataset_out_length, 1);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.dataset_out_index = 0;
                eichrecht.transaction_inner_state++;
            }
            return false;
        }

        case 3: { // request dataset from holding registers (max 120 registers in one go)
            uint16_t length = (eichrecht.dataset_out_length+1)/2 - eichrecht.dataset_out_index;
            if(length > 120) {
                length = 120;
            }
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7612+1 + eichrecht.dataset_out_index, length); // length here is in registers
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 4: { // read dataset from holding registers
            uint16_t length = (eichrecht.dataset_out_length+1)/2 - eichrecht.dataset_out_index;
            if(length > 120) {
                length = 120;
            }

            bool ret = meter_get_read_registers_response_string(MODBUS_FC_READ_HOLDING_REGISTERS, &eichrecht.dataset_out[eichrecht.dataset_out_index*2], length*2); // length here is in bytes
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.dataset_out_index += length;
                if(eichrecht.dataset_out_index < (eichrecht.dataset_out_length+1)/2) {
                    // More to read
                    eichrecht.transaction_inner_state = 3;
                    return false;
                } else {
                    // Done
                    eichrecht.transaction_inner_state = 0;
                    eichrecht.dataset_out_ready = true;
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

bool eichrecht_iskra_read_signature(void) {
    switch(eichrecht.transaction_inner_state) {
        case 0: {
            if (eichrecht.dataset_out_ready) {
                return false;
            }
            memset(eichrecht.signature, 0, sizeof(eichrecht.signature));
            eichrecht.signature_index = 0;
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // request signature length from holding register
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 7058+1, 1);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 2: { // read signature length from holding register
            bool ret = meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &eichrecht.signature_length, 1);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.signature_index = 0;
                eichrecht.transaction_inner_state++;
            }
            return false;
        }

        case 3: { // request signature from holding registers (max 120 registers in one go)
            uint16_t length = eichrecht.signature_length - eichrecht.signature_index;
            if(length > 120) {
                length = 120;
            }
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 8188+1 + eichrecht.signature_index, length);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 4: { // read signature from holding registers
            uint16_t length = eichrecht.signature_length - eichrecht.signature_index;
            if(length > 120) {
                length = 120;
            }

            bool ret = meter_get_read_registers_response_string(MODBUS_FC_READ_HOLDING_REGISTERS, &eichrecht.signature[eichrecht.signature_index], length);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.signature_index += length;
                if(eichrecht.signature_index < eichrecht.signature_length) {
                    // More to read
                    eichrecht.transaction_inner_state = 3;
                    return false;
                } else {
                    // Done
                    eichrecht.transaction_inner_state = 0;
                    eichrecht.signature_ready = true;
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

bool eichrecht_iskra_read_public_key(void) {
    switch(eichrecht.transaction_inner_state) {
        case 0: {
            memset(eichrecht.public_key, 0, sizeof(eichrecht.public_key));
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 1: { // request public key from holding registers (64 bytes fixed size)
            meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, 8124+1, 64);
            eichrecht.transaction_inner_state++;
            return false;
        }

        case 2: { // read public key from holding registers
            bool ret = meter_get_read_registers_response_string(MODBUS_FC_READ_HOLDING_REGISTERS, eichrecht.public_key, 64);
            if(ret) {
                modbus_clear_request(&rs485);
                eichrecht.transaction_inner_state = 0;
                return true;
            }
            return false;
        }
    }

    return false;
}

void eichrecht_reset_transaction(void) {
    eichrecht.transaction_state = 0;
    eichrecht.transaction_inner_state = 0;
    eichrecht.transaction_state_time = 0;
    modbus_clear_request(&rs485);
}

bool eichrecht_iskra_tick_next_state(void) {
    switch(eichrecht.transaction_state) {
        case 0: return false; // Should be unreachable
        case 1: return (eichrecht.transaction == 'B' || eichrecht.transaction == 'E' || eichrecht.transaction == 'r' || eichrecht.transaction == 'i') ? eichrecht_iskra_write_utc_time_offset(eichrecht.utc_time_offset) : true; // Only set utc time offset and unix time for 'B' transaction
        case 2: return (eichrecht.transaction == 'B' || eichrecht.transaction == 'E' || eichrecht.transaction == 'r' || eichrecht.transaction == 'i') ? eichrecht_iskra_write_unix_time(eichrecht.unix_time) : true;
        case 3: return eichrecht_iskra_write_signature_format(eichrecht.signature_format);
        case 4: return eichrecht_iskra_write_dataset();
        case 5: {
            if(eichrecht_iskra_get_measurement_status(&meter_iskra.measurement_status)) {
                if(eichrecht.transaction == 'B' || eichrecht.transaction == 'i') {
                    if(meter_iskra.measurement_status != 0) { // 0 = idle
                        // Should be idle
                        return false;
                    }
                } else if(eichrecht.transaction == 'E' || eichrecht.transaction == 'C' || eichrecht.transaction == 'X' || eichrecht.transaction == 'T' || eichrecht.transaction == 'S' || eichrecht.transaction == 'r' || eichrecht.transaction == 'h') {
                    if(meter_iskra.measurement_status != 1) { // 1 = active
                        // Should be active
                        return false;
                    }
                }
                return true;
            }
            return false;
        }
        case 6: return eichrecht_iskra_send_transaction_command(eichrecht.transaction);
        case 7: {
            if(eichrecht_iskra_get_signature_status(&meter_iskra.signature_status)) {
                if(meter_iskra.signature_status == 15) { // 15 = signature OK
                    return true;
                }
            }
            return false;
        }
        case 8: return eichrecht_iskra_read_dataset();
        case 9: return eichrecht_iskra_read_signature();
        default: eichrecht_reset_transaction();
    }

    return false;
}


// This is called by meter_iskra when eichrecht.init_done == false
void eichrecht_iskra_init_tick(void) {
    if(eichrecht_iskra_read_public_key()) {
        eichrecht.init_done = true;
    }
}

// This is called by meter_iskra when meter is idle and eichrecht.transaction_state > 0
void eichrecht_iskra_tick(void) {
    // Eichrecht support only on v4 hardware
    if(!hardware_version.is_v4) {
        return;
    }

    // TODO: How long for the timeout?
    // Check for timeout
    if(eichrecht.transaction_state_time == 0) {
        eichrecht.transaction_state_time = system_timer_get_ms();
    } else if(system_timer_is_time_elapsed_ms(eichrecht.transaction_state_time, 3000)) {
        eichrecht.timeout_counter++; // TODO: Add timeout counter to API?
        eichrecht_reset_transaction();
        return;
    }

    if(eichrecht_iskra_tick_next_state()) {
        eichrecht.transaction_state++;
        eichrecht.transaction_inner_state = 0;
        eichrecht.transaction_state_time = system_timer_get_ms();
    }
}
