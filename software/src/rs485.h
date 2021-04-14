/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * rs485.h: RS485 driver
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

#ifndef RS485_H
#define RS485_H

#include <stdbool.h>
#include <stdint.h>

#include "bricklib2/utility/ringbuffer.h"

#include "configs/config_rs485.h"

#define RS485_BUFFER_SIZE 1024*2

#define RS485_OVERSAMPLING 16

#define MODBUS_DEFAULT_SLAVE_ADDRESS 1
#define RS485_MODBUS_RTU_FRAME_SIZE_MAX 256
#define MODBUS_DEFAULT_MASTER_REQUEST_TIMEOUT 1000 // Milliseconds.

typedef enum {
	MODE_RS485 = 0,
	MODE_MODBUS_MASTER_RTU = 1,
	MODE_MODBUS_SLAVE_RTU = 2
} RS485Mode;

typedef enum {
	PARITY_NONE = 0,
	PARITY_ODD = 1,
	PARITY_EVEN = 2,
} RS485Parity;

typedef enum {
	STOPBITS_1 = 1,
	STOPBITS_2 = 2,
} RS485Stopbits;

typedef enum {
	WORDLENGTH_5 = 5,
	WORDLENGTH_6 = 6,
	WORDLENGTH_7 = 7,
	WORDLENGTH_8 = 8,
} RS485Wordlength;

typedef enum {
	DUPLEX_HALF = 0,
	DUPLEX_FULL = 1,
} RS485Duplex;

// TODO: Can we detect more errors? Framing error?
typedef enum {
	ERROR_OVERRUN = 1 << 0,
	ERROR_PARITY  = 1 << 1,
} RS485Error;

// Modbus specific.
typedef enum {
	MODBUS_RTU_WIRE_STATE_IDLE = 0,
	MODBUS_RTU_WIRE_STATE_RX = 1,
	MODBUS_RTU_WIRE_STATE_TX = 2,
} RS485ModbusRTUWireState;

typedef enum {
	MODBUS_REQUEST_PROCESS_STATE_READY = 0,
	MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE = 1,
	MODBUS_REQUEST_PROCESS_STATE_SLAVE_PROCESSING_REQUEST = 2,
} RS485ModbusRequestState;

typedef struct {
	bool in_progress;
	uint32_t chunk_total;
	uint32_t stream_total;
	uint32_t chunk_current;
	uint32_t stream_total_bool_array;
} RS485ModbusStreamChunking;

typedef struct {
	uint8_t id;
	bool cb_invoke;
	uint16_t length;
	uint8_t *rx_frame;
	uint8_t *tx_frame;
	RS485ModbusRequestState state;
	bool master_request_timed_out;
	uint32_t time_ref_master_request_timeout;
	RS485ModbusStreamChunking *stream_chunking;
} RS485ModbusRequest;

typedef struct {
	uint32_t timeout;
	uint32_t checksum;
	uint32_t frame_too_big;
	uint32_t illegal_function;
	uint32_t illegal_data_value;
	uint32_t illegal_data_address;
	uint32_t slave_device_failure;
} RS485ModbusCommonErrorCounters;

typedef struct {
	bool tx_done;
	uint32_t time_4_chars_us;
	uint16_t rx_rb_last_length;
	RS485ModbusRequest request;
	RS485ModbusRTUWireState state_wire;
} RS485ModbusRTU;

typedef struct {
	RS485Mode mode;
	uint32_t baudrate;
	RS485Parity parity;
	RS485Duplex duplex;
	RS485Stopbits stopbits;
	uint16_t buffer_size_rx;
	Ringbuffer ringbuffer_tx;
	Ringbuffer ringbuffer_rx;
	bool read_callback_enabled;
	uint16_t frame_readable_cb_frame_size;
	bool frame_readable_cb_already_sent;
	RS485Wordlength wordlength;
	uint32_t error_count_parity;
	uint32_t error_count_overrun;
	uint32_t duplex_shift_setting;
	bool error_count_callback_enabled;
	uint8_t buffer[RS485_BUFFER_SIZE];

	// Modbus specific.
	RS485ModbusRTU modbus_rtu;
	uint8_t modbus_slave_address;
	uint32_t modbus_master_request_timeout;
	RS485ModbusCommonErrorCounters modbus_common_error_counters;
} RS485;

extern RS485 rs485;

void rs485_init(void);
void rs485_tick(void);

#endif