/* evse-v2-bricklet
 * Copyright (C) 2017 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * modbus.c: Modbus implementation (originally from rs485-bricklet)
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

#include "modbus.h"

#include "timer.h"

#include "bricklib2/utility/crc16.h"
#include "bricklib2/hal/system_timer/system_timer.h"

#include "xmc_uart.h"
#include "xmc_scu.h"

RS485ModbusStreamChunking modbus_stream_chunking;

void modbus_init(RS485 *rs485) {
	if(rs485->baudrate > 19200) {
		rs485->modbus_rtu.time_4_chars_us = 1750;
	}
	else {
		// Always one start bit.
		uint8_t bit_count = 1;

		bit_count += rs485->wordlength;

		if(rs485->parity != PARITY_NONE) {
			bit_count++;
		}

		bit_count += rs485->stopbits;

		rs485->modbus_rtu.time_4_chars_us = (4 * bit_count * 1000000) / rs485->baudrate;
	}

	rs485->modbus_rtu.request.id = 1;
	rs485->modbus_rtu.tx_done = false;
	rs485->modbus_rtu.request.length = 0;
	rs485->modbus_rtu.rx_rb_last_length = 0;
	rs485->modbus_rtu.request.cb_invoke = false;
	rs485->modbus_rtu.request.rx_frame = &rs485->buffer[0];
	rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_IDLE;
	rs485->modbus_rtu.request.master_request_timed_out = false;
	rs485->modbus_rtu.request.state = MODBUS_REQUEST_PROCESS_STATE_READY;
	rs485->modbus_rtu.request.tx_frame = &rs485->buffer[rs485->buffer_size_rx];

	memset(rs485->modbus_rtu.request.rx_frame, 0, rs485->buffer_size_rx);
	memset(rs485->modbus_rtu.request.tx_frame, 0, RS485_BUFFER_SIZE - rs485->buffer_size_rx);
	memset(&rs485->modbus_common_error_counters, 0, sizeof(RS485ModbusCommonErrorCounters));
	memset(&modbus_stream_chunking, 0, sizeof(RS485ModbusStreamChunking));

	rs485->modbus_rtu.request.stream_chunking = &modbus_stream_chunking;
}

void modbus_clear_request(RS485 *rs485) {
	rs485->modbus_rtu.request.id++;

	ringbuffer_init(&rs485->ringbuffer_rx, rs485->buffer_size_rx, &rs485->buffer[0]);
	ringbuffer_init(&rs485->ringbuffer_tx, RS485_BUFFER_SIZE-rs485->buffer_size_rx, &rs485->buffer[rs485->buffer_size_rx]);

	memset(&modbus_stream_chunking, 0, sizeof(RS485ModbusStreamChunking));

	rs485->modbus_rtu.request.length = 0;
	rs485->modbus_rtu.rx_rb_last_length = 0;
	rs485->modbus_rtu.request.cb_invoke = false;
	rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_IDLE;
	rs485->modbus_rtu.request.master_request_timed_out = false;
	rs485->modbus_rtu.request.state = MODBUS_REQUEST_PROCESS_STATE_READY;
}

bool modbus_slave_check_address(RS485 *rs485) {
	if(rs485->modbus_rtu.request.rx_frame[0] == rs485->modbus_slave_address) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[0] == 0) {
		// Broadcast.
		return true;
	}
	else {
		return false;
	}
}

void modbus_start_tx_from_buffer(RS485 *rs485) {
	rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_TX;

	XMC_USIC_CH_TXFIFO_EnableEvent(RS485_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);
	XMC_USIC_CH_TriggerServiceRequest(RS485_USIC, RS485_SERVICE_REQUEST_TX);
}

bool modbus_check_frame_checksum(RS485 *rs485) {
	if(rs485->modbus_rtu.rx_rb_last_length < 2) {
		return false;
	}

	uint16_t checksum = crc16_modbus(rs485->modbus_rtu.request.rx_frame,
	                                 rs485->modbus_rtu.rx_rb_last_length - 2);

	uint8_t *checksum_recvd = &rs485->modbus_rtu.request.rx_frame[rs485->modbus_rtu.rx_rb_last_length - 2];

	return (checksum_recvd[0] == (checksum & 0xFF) && checksum_recvd[1] == (checksum >> 8));
}

bool modbus_master_check_slave_response(RS485 *rs485) {
	// Check slave address.
	if(rs485->modbus_rtu.request.rx_frame[0] != rs485->modbus_rtu.request.tx_frame[0]) {
		return false;
	}

	// Check the function code in response.
	if((rs485->modbus_rtu.request.rx_frame[1] != rs485->modbus_rtu.request.tx_frame[1])) {
		if(rs485->modbus_rtu.request.rx_frame[1] != (rs485->modbus_rtu.request.tx_frame[1] + 0x80)) {
			return false;
		}
	}

	return true;
}

void modbus_update_rtu_wire_state_machine(RS485 *rs485) {
	uint16_t new_bytes = 0;

	if(rs485->modbus_rtu.state_wire == MODBUS_RTU_WIRE_STATE_IDLE) {
		// In slave mode.
		if(rs485->mode == MODE_MODBUS_SLAVE_RTU) {
			// Check if slave request has timedout.
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_SLAVE_PROCESSING_REQUEST) &&
				 (rs485->modbus_rtu.request.length < ringbuffer_get_used(&rs485->ringbuffer_rx))) {
				/*
				 * While handling request timeout in slave mode, interrupts are disabled
				 * because ringbuffer_add() is updated which is also updated in the RX IRQ.
				 */
				NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_TFF);
				NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_TX);
				NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RX);
				NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RXA);
				__DSB();
				__ISB();

				rs485->modbus_common_error_counters.timeout++;

				new_bytes = \
					ringbuffer_get_used(&rs485->ringbuffer_rx) - rs485->modbus_rtu.request.length;

				modbus_clear_request(rs485);

				// Move the available bytes to the beginning of the Rx buffer.
				if(new_bytes <= RS485_MODBUS_RTU_FRAME_SIZE_MAX) {
					for(uint16_t i = rs485->modbus_rtu.request.length; i < new_bytes; i++) {
						ringbuffer_add(&rs485->ringbuffer_rx, rs485->ringbuffer_rx.buffer[i]);
					}
				}

				rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_RX;

				NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TFF);
				NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TX);
				NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RX);
				NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RXA);

				return;
			}

			// Check if there is new data in the RX buffer.
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_READY) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > 0) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > rs485->modbus_rtu.rx_rb_last_length)) {

				rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_RX;
				rs485->modbus_rtu.rx_rb_last_length = ringbuffer_get_used(&rs485->ringbuffer_rx);

				return;
			}

			// Check if the a frame is available in the RX buffer.
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_READY) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > 0) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) == rs485->modbus_rtu.rx_rb_last_length)) {
				if(ringbuffer_get_used(&rs485->ringbuffer_rx) > RS485_MODBUS_RTU_FRAME_SIZE_MAX) {
					// Frame is too big.
					rs485->modbus_common_error_counters.frame_too_big++;

					modbus_clear_request(rs485);

					return;
				}

				if(!modbus_check_frame_checksum(rs485)) {
					// Frame checksum mismatch.
					rs485->modbus_common_error_counters.checksum++;

					modbus_clear_request(rs485);

					return;
				}

				if(!modbus_slave_check_address(rs485)) {
					// The frame is not for this slave.
					modbus_clear_request(rs485);

					return;
				}

				if(!modbus_slave_check_function_code_imlemented(rs485)) {
					// The function is not implemented by the bricklet.
					rs485->modbus_common_error_counters.illegal_function++;

					modbus_report_exception(rs485, rs485->modbus_rtu.request.rx_frame[1], MODBUS_EC_ILLEGAL_FUNCTION);

					return;
				}

				modbus_init_new_request(rs485,
				                        MODBUS_REQUEST_PROCESS_STATE_SLAVE_PROCESSING_REQUEST,
				                        ringbuffer_get_used(&rs485->ringbuffer_rx));

				return;
			}
		}
		// In master mode.
		else if(rs485->mode == MODE_MODBUS_MASTER_RTU) {
			// Check if master request has timedout.
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) &&
			   system_timer_is_time_elapsed_ms(rs485->modbus_rtu.request.time_ref_master_request_timeout,
			                                   rs485->modbus_master_request_timeout) &&
			   !rs485->modbus_rtu.request.master_request_timed_out) {
			  rs485->modbus_common_error_counters.timeout++;

				/*
				 * Set master request timeout flag. Which will be observed by the
				 * corresponding callback handler and processed further.
				 */
				rs485->modbus_rtu.request.cb_invoke = true;
				rs485->modbus_rtu.request.master_request_timed_out = true;

				return;
			}

			/*
			 * Check if data from the salve response has started to appear in
			 * the buffer. Change to RX state if needed.
			 */
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > 0) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > rs485->modbus_rtu.rx_rb_last_length)) {
				rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_RX;
				rs485->modbus_rtu.rx_rb_last_length = ringbuffer_get_used(&rs485->ringbuffer_rx);

				return;
			}

			// Check if a response frame is in buffer, if so then process it.
			if((rs485->modbus_rtu.request.state == MODBUS_REQUEST_PROCESS_STATE_MASTER_WAITING_RESPONSE) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) > 0) &&
				 (ringbuffer_get_used(&rs485->ringbuffer_rx) == rs485->modbus_rtu.rx_rb_last_length)) {
				if(rs485->modbus_rtu.request.stream_chunking->in_progress) {
					// In process of streaming chunks of slave response to the user.
					return;
				}

				if(ringbuffer_get_used(&rs485->ringbuffer_rx) > RS485_MODBUS_RTU_FRAME_SIZE_MAX &&
				   !rs485->modbus_rtu.request.master_request_timed_out) {
					// Frame is too big.
					rs485->modbus_common_error_counters.timeout++;
					rs485->modbus_common_error_counters.frame_too_big++;

					rs485->modbus_rtu.request.cb_invoke = true;
					rs485->modbus_rtu.request.master_request_timed_out = true;

					return;
				}

				if(!modbus_check_frame_checksum(rs485) &&
				   !rs485->modbus_rtu.request.master_request_timed_out) {
					// Frame checksum mismatch.
					rs485->modbus_common_error_counters.timeout++;
					rs485->modbus_common_error_counters.checksum++;

					/*
					 * Since the master is waiting for a valid response from the slave and
					 * we received mangled response hence this is handled by generating a
					 * request timeout callback.
					 */
					rs485->modbus_rtu.request.cb_invoke = true;
					rs485->modbus_rtu.request.master_request_timed_out = true;

					return;
				}

				if(!modbus_master_check_slave_response(rs485) &&
				   !rs485->modbus_rtu.request.master_request_timed_out) {
					// The frame is not from expected slave.
					rs485->modbus_common_error_counters.timeout++;

					/*
					 * Since the master is waiting for a valid response from the slave and
					 * we received mangled response hence this is handled by generating a
					 * request timeout callback.
					 */
					rs485->modbus_rtu.request.cb_invoke = true;
					rs485->modbus_rtu.request.master_request_timed_out = true;

					return;
				}

				/*
				 * We have a valid response from a slave for the master request.
				 * The response will be handled with callbacks.
				 */
				rs485->modbus_rtu.request.cb_invoke = true;

				return;
			}
		}
	}
	else if(rs485->modbus_rtu.state_wire == MODBUS_RTU_WIRE_STATE_RX) {
		if(timer_us_elapsed_since_last_timer_reset(rs485->modbus_rtu.time_4_chars_us)) {
			rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_IDLE;
		}
	}
	else if(rs485->modbus_rtu.state_wire == MODBUS_RTU_WIRE_STATE_TX) {
		if(rs485->modbus_rtu.tx_done) {
			rs485->modbus_rtu.tx_done = false;
			rs485->modbus_rtu.state_wire = MODBUS_RTU_WIRE_STATE_IDLE;

			if(rs485->mode == MODE_MODBUS_SLAVE_RTU) {
				// In slave mode completed TX means end of a request handling.
				modbus_clear_request(rs485);
			}

			if(rs485->mode == MODE_MODBUS_MASTER_RTU && rs485->modbus_rtu.request.tx_frame[0] == 0) {
				// In master mode generate a callback response for broadcast requests.
				rs485->modbus_rtu.request.cb_invoke = true;
			}
		}
	}
}

bool modbus_slave_check_function_code_imlemented(RS485 *rs485) {
	if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_READ_COILS) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_READ_HOLDING_REGISTERS) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_WRITE_SINGLE_COIL) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_WRITE_SINGLE_REGISTER) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_WRITE_MULTIPLE_COILS) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_WRITE_MULTIPLE_REGISTERS) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_READ_DISCRETE_INPUTS) {
		return true;
	}
	else if(rs485->modbus_rtu.request.rx_frame[1] == MODBUS_FC_READ_INPUT_REGISTERS) {
		return true;
	}
	else {
		return false;
	}
}

void modbus_init_new_request(RS485 *rs485, RS485ModbusRequestState state, uint16_t length) {
	if(rs485->modbus_rtu.request.id == 255) {
		rs485->modbus_rtu.request.id = 1;
	}
	else {
		rs485->modbus_rtu.request.id++;
	}

	rs485->modbus_rtu.request.state = state;
	rs485->modbus_rtu.request.length = length;

	if(rs485->mode == MODE_MODBUS_SLAVE_RTU) {
		rs485->modbus_rtu.request.cb_invoke = true;
	}
	else if(rs485->mode == MODE_MODBUS_MASTER_RTU) {
		rs485->modbus_rtu.request.cb_invoke = false;
	}
}

void modbus_report_exception(RS485 *rs485, uint8_t function_code, ModbusExceptionCode exception_code) {
	ModbusExceptionResponse modbus_exception_response;
	uint8_t *modbus_exception_response_ptr = (uint8_t *)&modbus_exception_response;

	modbus_exception_response.address = rs485->modbus_slave_address;
	modbus_exception_response.function_code = function_code + 0x80;
	modbus_exception_response.exception_code = (uint8_t)exception_code;
	modbus_exception_response.checksum = crc16_modbus(modbus_exception_response_ptr,
	                                                  sizeof(ModbusExceptionResponse) - 2);

	ringbuffer_init(&rs485->ringbuffer_tx,
	                RS485_BUFFER_SIZE - rs485->buffer_size_rx,
	                &rs485->buffer[rs485->buffer_size_rx]);

	for(uint8_t i = 0; i < sizeof(ModbusExceptionResponse); i++) {
		ringbuffer_add(&rs485->ringbuffer_tx, modbus_exception_response_ptr[i]);
	}

	modbus_start_tx_from_buffer(rs485);
}
