/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iskra_display.c: Custom text and backlight control for Iskra WM3M4(C) LCD
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

#include "iskra_display.h"

#include <string.h>

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/warp/meter.h"
#include "bricklib2/warp/modbus.h"
#include "bricklib2/warp/rs485.h"

#include "communication.h"
#include "iec61851.h"
#include "button.h"

#define ISKRA_DISPLAY_REG_BACKLIGHT     (7061+1) // On/Off
#define ISKRA_DISPLAY_REG_LCD_PARAMS    (7062+1) // Row 2 mode bitmask
#define ISKRA_DISPLAY_REG_CUSTOM_STRING (7063+1) // 8 chars (4 registers)
#define ISKRA_DISPLAY_REG_CUSTOM_LABEL  (7067+1) // 4 chars (2 registers)
#define ISKRA_DISPLAY_REG_OPERATOR_COMMAND (12+1)
#define ISKRA_DISPLAY_REG_CHANGE_LOCK      (7097+1)
#define ISKRA_DISPLAY_REG_CHANGE_SETTING   (7098+1)

#define ISKRA_DISPLAY_LCD_PARAMS_CUSTOM_STRING (1 << 3)

// WM3M4 LCD params are saved before permanently locking the configuration.
// WM3M4C display changes remain volatile.

#define ISKRA_DISPLAY_TIMEOUT 3000
#define ISKRA_DISPLAY_RETRY_DELAY 10000

IskraDisplay iskra_display;

void iskra_display_init(void) {
	memset(&iskra_display, 0, sizeof(IskraDisplay));
	iskra_display.backlight_mode = EVSE_V2_ENERGY_METER_DISPLAY_BACKLIGHT_AUTOMATIC;
}

static bool iskra_display_backlight_write_needed(void) {
	return !iskra_display.backlight_written_valid || (iskra_display.backlight_written != iskra_display.backlight_desired);
}

void iskra_display_tick(void) {
	const uint32_t t = system_timer_get_ms();

	if(meter.type != iskra_display.meter_type_last) {
		iskra_display.meter_type_last = (uint8_t)meter.type;
		if((meter.type == METER_TYPE_WM3M4) || (meter.type == METER_TYPE_WM3M4C)) {
			iskra_display.backlight_written_valid = false;
			iskra_display.text_pending = true;
		}
	}

	const bool charging = (iec61851.state == IEC61851_STATE_C) || (iec61851.state == IEC61851_STATE_D);
	if(iskra_display.charging_last && !charging) {
		iskra_display.event_time = t;
	}
	iskra_display.charging_last = charging;

	const bool pressed = (button.state == BUTTON_STATE_PRESSED) || (button.state == BUTTON_STATE_PRESSED_DEBOUNCE);
	if(pressed && !iskra_display.button_pressed_last) {
		iskra_display.event_time = t;
	}
	iskra_display.button_pressed_last = pressed;

	switch(iskra_display.backlight_mode) {
		case EVSE_V2_ENERGY_METER_DISPLAY_BACKLIGHT_OFF: {
			iskra_display.backlight_desired = false;
			break;
		}

		case EVSE_V2_ENERGY_METER_DISPLAY_BACKLIGHT_ON: {
			iskra_display.backlight_desired = true;
			break;
		}

		case EVSE_V2_ENERGY_METER_DISPLAY_BACKLIGHT_AUTOMATIC:
		default: {
			// Backlight is on while an EV is charging and for 5 minutes after the last event.
			iskra_display.backlight_desired = charging || ((iskra_display.event_time != 0) && !system_timer_is_time_elapsed_ms(iskra_display.event_time, ISKRA_DISPLAY_BACKLIGHT_AUTO_OFF_TIME));
			break;
		}
	}
}

bool iskra_display_has_work(void) {
	if(iskra_display.retry_pending && !system_timer_is_time_elapsed_ms(iskra_display.retry_time, ISKRA_DISPLAY_RETRY_DELAY)) {
		return false;
	}
	return (iskra_display.state != 0) || iskra_display.text_pending || iskra_display.lock_pending || iskra_display_backlight_write_needed();
}

static void iskra_display_retry(void) {
	modbus_clear_request(&rs485);
	iskra_display.state = 0;
	iskra_display.retry_pending = true;
	iskra_display.retry_time = system_timer_get_ms();
}

static void iskra_display_read(const uint16_t reg, const uint8_t next_state) {
	meter_read_registers(MODBUS_FC_READ_HOLDING_REGISTERS, meter.slave_address, reg, 1);
	iskra_display.state_time = system_timer_get_ms();
	iskra_display.state = next_state;
}

static void iskra_display_write(const uint16_t reg, const uint16_t value, const uint8_t next_state) {
	MeterRegisterType payload;
	payload.u16_single = value;
	meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, reg, &payload);
	iskra_display.state_time = system_timer_get_ms();
	iskra_display.state = next_state;
}

void iskra_display_modbus_tick(void) {
	if(iskra_display.retry_pending) {
		if(!system_timer_is_time_elapsed_ms(iskra_display.retry_time, ISKRA_DISPLAY_RETRY_DELAY)) {
			return;
		}
		iskra_display.retry_pending = false;
	}

	if(iskra_display.state != 0) {
		// The meter response helpers report completion, including failed requests.
		// Handle errors before consuming a response or advancing towards the lock.
		if(rs485.modbus_rtu.request.cb_invoke &&
		   (rs485.modbus_rtu.request.master_request_timed_out ||
		    (rs485.modbus_rtu.request.rx_frame[1] == (rs485.modbus_rtu.request.tx_frame[1] + 0x80)))) {
			// Count the failed response using the common meter response handler.
			meter_get_write_register_response(rs485.modbus_rtu.request.tx_frame[1]);
			iskra_display_retry();
			return;
		}
		if(system_timer_is_time_elapsed_ms(iskra_display.state_time, ISKRA_DISPLAY_TIMEOUT)) {
			iskra_display_retry();
			return;
		}
	}

	switch(iskra_display.state) {
		case 0: { // idle -> start next operation
			iskra_display.state_time = system_timer_get_ms();
			if(iskra_display_backlight_write_needed()) {
				MeterRegisterType payload;
				payload.u16_single = iskra_display.backlight_desired ? 1 : 0;
				meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, ISKRA_DISPLAY_REG_BACKLIGHT, &payload);
				iskra_display.state = 1;
			} else if(iskra_display.text_pending) {
				meter_write_string(meter.slave_address, ISKRA_DISPLAY_REG_CUSTOM_STRING, iskra_display.text, ISKRA_DISPLAY_TEXT_LENGTH);
				iskra_display.state = 2;
			} else if(iskra_display.lock_pending) {
				if(meter.type == METER_TYPE_WM3M4) {
					iskra_display_read(ISKRA_DISPLAY_REG_LCD_PARAMS, 5);
				} else {
					iskra_display.lock_pending = false;
				}
			}
			break;
		}

		case 1: { // check backlight write response
			if(meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER)) {
				modbus_clear_request(&rs485);
				iskra_display.backlight_written       = iskra_display.backlight_desired;
				iskra_display.backlight_written_valid = true;
				iskra_display.state                   = 0;
			}
			break;
		}

		case 2: { // check custom string write response -> write label
			if(meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS)) {
				modbus_clear_request(&rs485);
				meter_write_string(meter.slave_address, ISKRA_DISPLAY_REG_CUSTOM_LABEL, iskra_display.label, ISKRA_DISPLAY_LABEL_LENGTH);
				iskra_display.state_time = system_timer_get_ms();
				iskra_display.state = 3;
			}
			break;
		}

		case 3: { // check label write response -> write LCD params
			if(meter_get_write_register_response(MODBUS_FC_WRITE_MULTIPLE_REGISTERS)) {
				modbus_clear_request(&rs485);

				// Always show the custom string on row 2, including an empty string
				// by default. Clearing the text must not change the locked LCD mode.
				MeterRegisterType payload;
				payload.u16_single = ISKRA_DISPLAY_LCD_PARAMS_CUSTOM_STRING;
				meter_write_register(MODBUS_FC_WRITE_SINGLE_REGISTER, meter.slave_address, ISKRA_DISPLAY_REG_LCD_PARAMS, &payload);
				iskra_display.state_time = system_timer_get_ms();
				iskra_display.state = 4;
			}
			break;
		}

		case 4: { // check LCD params write response
			if(meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER)) {
				modbus_clear_request(&rs485);
				iskra_display.text_pending = false;
				iskra_display.lock_pending = (meter.type == METER_TYPE_WM3M4);
				iskra_display.state = 0;
			}
			break;
		}

		case 5: { // verify that only custom text is selected before locking
			uint16_t mode = 0;
			if(meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &mode, 1)) {
				modbus_clear_request(&rs485);
				if(mode == ISKRA_DISPLAY_LCD_PARAMS_CUSTOM_STRING) {
					iskra_display_read(ISKRA_DISPLAY_REG_CHANGE_LOCK, 6);
				} else {
					iskra_display.lock_pending = false;
					iskra_display.state = 0;
				}
			}
			break;
		}

		case 6: { // check lock status on every attempt, including retries
			uint16_t locked = 0;
			if(meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &locked, 1)) {
				modbus_clear_request(&rs485);
				if((locked == 0) && (meter.type == METER_TYPE_WM3M4) && !iskra_display.text_pending) {
					// 47062 is not automatically saved by Change Lock. Persist it
					// first, otherwise a power cycle could restore an immutable old mode.
					iskra_display_write(ISKRA_DISPLAY_REG_OPERATOR_COMMAND, 1, 7);
				} else {
					iskra_display.lock_pending = false;
					iskra_display.state = 0;
				}
			}
			break;
		}

		case 7: { // settings saved -> enable Change Lock writes for 60 seconds
			if(meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER)) {
				modbus_clear_request(&rs485);
				iskra_display_write(ISKRA_DISPLAY_REG_CHANGE_SETTING, 12345, 8);
			}
			break;
		}

		case 8: { // password accepted -> permanently lock WM3M4 configuration
			if(meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER)) {
				modbus_clear_request(&rs485);
				if((meter.type == METER_TYPE_WM3M4) && !iskra_display.text_pending) {
					iskra_display_write(ISKRA_DISPLAY_REG_CHANGE_LOCK, 1, 9);
				} else {
					iskra_display.lock_pending = false;
					iskra_display.state = 0;
				}
			}
			break;
		}

		case 9: { // confirm the lock, rather than relying only on the write response
			if(meter_get_write_register_response(MODBUS_FC_WRITE_SINGLE_REGISTER)) {
				modbus_clear_request(&rs485);
				iskra_display_read(ISKRA_DISPLAY_REG_CHANGE_LOCK, 10);
			}
			break;
		}

		case 10: {
			uint16_t locked = 0;
			if(meter_get_read_registers_response(MODBUS_FC_READ_HOLDING_REGISTERS, &locked, 1)) {
				if(locked != 1) {
					iskra_display_retry();
				} else {
					modbus_clear_request(&rs485);
					iskra_display.lock_pending = false;
					iskra_display.state = 0;
				}
			}
			break;
		}

		default: {
			iskra_display.state = 0;
			break;
		}
	}
}
