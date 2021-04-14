/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * rs485.c: RS485 driver
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

#include "rs485.h"
#include "configs/config_rs485.h"

#include "modbus.h"
#include "timer.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/ringbuffer.h"

RS485 rs485;

#define rs485_rx_irq_handler  IRQ_Hdlr_11
#define rs485_tx_irq_handler  IRQ_Hdlr_12
#define rs485_tff_irq_handler IRQ_Hdlr_13
#define rs485_rxa_irq_handler IRQ_Hdlr_14

#define RS485_HALF_DUPLEX_RX_ENABLE() \
	(RS485_NRXE_AND_TXE_PORT->OMR = ((0x10000 << RS485_TXE_PIN_NUM) | (0x10000 << RS485_NRXE_PIN_NUM)))

// Use duplex_shift_setting to not turn rx off in full-duplex
#define RS485_HALF_DUPLEX_TX_ENABLE() \
	(RS485_NRXE_AND_TXE_PORT->OMR = ((0x1 << RS485_TXE_PIN_NUM) | (rs485.duplex_shift_setting << RS485_NRXE_PIN_NUM)))

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs485_tff_irq_handler(void) {
	if((RS485_USIC->PSR_ASCMode & (XMC_UART_CH_STATUS_FLAG_TRANSMITTER_FRAME_FINISHED | XMC_UART_CH_STATUS_FLAG_TRANSFER_STATUS_BUSY)) ==
		 XMC_UART_CH_STATUS_FLAG_TRANSMITTER_FRAME_FINISHED) {
			 RS485_HALF_DUPLEX_RX_ENABLE();
	}
}

// Set const pointer to rx ringbuffer variables.
// With this the compiler can properly optimize the access!
uint8_t  *const rs485_ringbuffer_rx_buffer = &(rs485.buffer[0]); // rx part of buffer always starts from 0, we can't do the same trick for tx!!!
uint16_t *const rs485_ringbuffer_rx_end    = &(rs485.ringbuffer_rx.end);
uint16_t *const rs485_ringbuffer_rx_start  = &(rs485.ringbuffer_rx.start);
uint16_t *const rs485_ringbuffer_rx_size   = &(rs485.ringbuffer_rx.size);
void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs485_rx_irq_handler(void) {
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(RS485_USIC)) {
		// Instead of ringbuffer_add() we add the byte to the buffer
		// by hand.
		//
		// We need to save the low watermark calculation overhead.

		uint16_t new_end = *rs485_ringbuffer_rx_end + 1;

		if(new_end >= *rs485_ringbuffer_rx_size) {
			new_end = 0;
		}

		if(new_end == *rs485_ringbuffer_rx_start) {
			rs485.error_count_overrun++;

			// In the case of an overrun we read the byte and throw it away.
			volatile uint8_t __attribute__((unused)) _  = RS485_USIC->OUTR;
		} else {
			rs485_ringbuffer_rx_buffer[*rs485_ringbuffer_rx_end] = RS485_USIC->OUTR;
			*rs485_ringbuffer_rx_end = new_end;
		}
	}

	TIMER_RESET();
}


void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs485_rxa_irq_handler(void) {
	// We get alternate rx interrupt if there is a parity error. In this case we
	// still read the byte and give it to the user.
	rs485.error_count_parity++;

	rs485_rx_irq_handler();
}

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) rs485_tx_irq_handler(void) {
	RS485_HALF_DUPLEX_TX_ENABLE();

	while(!XMC_USIC_CH_TXFIFO_IsFull(RS485_USIC)) {
		// TX FIFO is not full, more data can be loaded on the FIFO from the ring buffer.
		uint8_t data;
		if(!ringbuffer_get(&rs485.ringbuffer_tx, &data)) {
			// No more data to TX from ringbuffer, disable TX interrupt.
			XMC_USIC_CH_TXFIFO_DisableEvent(RS485_USIC, XMC_USIC_CH_TXFIFO_EVENT_CONF_STANDARD);

			rs485.modbus_rtu.tx_done = true;

			return;
		}

		RS485_USIC->IN[0] = data;
	}
}

void rs485_init_hardware(void) {
	// TX pin configuration
	const XMC_GPIO_CONFIG_t tx_pin_config = {
		.mode             = RS485_TX_PIN_AF,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_HIGH
	};

	// RX pin configuration
	const XMC_GPIO_CONFIG_t rx_pin_config = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	// TXE (tx enable) pin configuration
	const XMC_GPIO_CONFIG_t txe_pin_config = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	// NRXE (not rx enable) pin configuration
	const XMC_GPIO_CONFIG_t nrxe_pin_config = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	// Configure  pins
	XMC_GPIO_Init(RS485_TX_PIN, &tx_pin_config);
	XMC_GPIO_Init(RS485_RX_PIN, &rx_pin_config);
	XMC_GPIO_Init(RS485_TXE_PIN, &txe_pin_config);
	XMC_GPIO_Init(RS485_NRXE_PIN, &nrxe_pin_config);

	// Initialize USIC channel in UART master mode
	// USIC channel configuration
	XMC_UART_CH_CONFIG_t config;
	config.oversampling = RS485_OVERSAMPLING;
	config.frame_length = rs485.wordlength;
	config.baudrate     = rs485.baudrate;
	config.stop_bits    = rs485.stopbits;
	config.data_bits    = rs485.wordlength;
	switch(rs485.parity) {
		case PARITY_NONE: config.parity_mode = XMC_USIC_CH_PARITY_MODE_NONE; break;
		case PARITY_EVEN: config.parity_mode = XMC_USIC_CH_PARITY_MODE_EVEN; break;
		case PARITY_ODD:  config.parity_mode = XMC_USIC_CH_PARITY_MODE_ODD;  break;
	}
	XMC_UART_CH_Init(RS485_USIC, &config);

	// Set input source path
	XMC_UART_CH_SetInputSource(RS485_USIC, RS485_RX_INPUT, RS485_RX_SOURCE);

	// Configure transmit FIFO
	XMC_USIC_CH_TXFIFO_Configure(RS485_USIC, 32, XMC_USIC_CH_FIFO_SIZE_32WORDS, 16);

	// Configure receive FIFO
	XMC_USIC_CH_RXFIFO_Configure(RS485_USIC, 0, XMC_USIC_CH_FIFO_SIZE_32WORDS, 16);

	// UART protocol events
	XMC_USIC_CH_SetInterruptNodePointer(RS485_USIC, XMC_USIC_CH_INTERRUPT_NODE_POINTER_PROTOCOL, RS485_SERVICE_REQUEST_TFF);
	XMC_USIC_CH_SetInterruptNodePointer(RS485_USIC, XMC_USIC_CH_INTERRUPT_NODE_POINTER_ALTERNATE_RECEIVE, RS485_SERVICE_REQUEST_RXA);
	XMC_UART_CH_EnableEvent(RS485_USIC, XMC_UART_CH_EVENT_FRAME_FINISHED);

	// Set service request for tx FIFO transmit interrupt
	XMC_USIC_CH_TXFIFO_SetInterruptNodePointer(RS485_USIC, XMC_USIC_CH_TXFIFO_INTERRUPT_NODE_POINTER_STANDARD, RS485_SERVICE_REQUEST_TX);

	// Set service request for rx FIFO receive interrupt
	XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(RS485_USIC, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_STANDARD, RS485_SERVICE_REQUEST_RX);
	XMC_USIC_CH_RXFIFO_SetInterruptNodePointer(RS485_USIC, XMC_USIC_CH_RXFIFO_INTERRUPT_NODE_POINTER_ALTERNATE, RS485_SERVICE_REQUEST_RX);

	// Set priority and enable NVIC node for transfer frame finished interrupt
	NVIC_SetPriority((IRQn_Type)RS485_IRQ_TFF, RS485_IRQ_TFF_PRIORITY);
	XMC_SCU_SetInterruptControl(RS485_IRQ_TFF, RS485_IRQCTRL_TFF);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TFF);

	// Set priority and enable NVIC node for transmit interrupt
	NVIC_SetPriority((IRQn_Type)RS485_IRQ_TX, RS485_IRQ_TX_PRIORITY);
	XMC_SCU_SetInterruptControl(RS485_IRQ_TX, RS485_IRQCTRL_TX);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TX);

	// Set priority and enable NVIC node for receive interrupt
	NVIC_SetPriority((IRQn_Type)RS485_IRQ_RX, RS485_IRQ_RX_PRIORITY);
	XMC_SCU_SetInterruptControl(RS485_IRQ_RX, RS485_IRQCTRL_RX);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RX);

	// Set priority and enable NVIC node for receive interrupt
	NVIC_SetPriority((IRQn_Type)RS485_IRQ_RXA, RS485_IRQ_RXA_PRIORITY);
	XMC_SCU_SetInterruptControl(RS485_IRQ_RXA, RS485_IRQCTRL_RXA);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RXA);

	// Start UART
	XMC_UART_CH_Start(RS485_USIC);

	XMC_USIC_CH_EnableEvent(RS485_USIC, XMC_USIC_CH_EVENT_ALTERNATIVE_RECEIVE);
	XMC_USIC_CH_RXFIFO_EnableEvent(RS485_USIC, XMC_USIC_CH_RXFIFO_EVENT_CONF_STANDARD | XMC_USIC_CH_RXFIFO_EVENT_CONF_ALTERNATE);
}

void rs485_init_buffer(void) {
	// Disable interrupts so we can't accidentally
	// receive ringbuffer_adds in between a re-init
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_TFF);
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_TX);
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RX);
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RXA);
	__DSB();
	__ISB();

	// Initialize rs485 buffer
	memset(rs485.buffer, 0, RS485_BUFFER_SIZE);

	// rx buffer is at buffer[0:buffer_size_rx]
	ringbuffer_init(&rs485.ringbuffer_rx, rs485.buffer_size_rx, &rs485.buffer[0]);
	// tx buffer is at buffer[buffer_size_rx:RS485_BUFFER_SIZE]
	ringbuffer_init(&rs485.ringbuffer_tx, RS485_BUFFER_SIZE-rs485.buffer_size_rx, &rs485.buffer[rs485.buffer_size_rx]);

	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TFF);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_TX);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RX);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RXA);
}

void rs485_init(void) {
	// Default config is Modbus RTU mode, 9600 baud, 8N1, half-duplex.
	rs485.mode                         = MODE_MODBUS_MASTER_RTU;
	rs485.baudrate                     = 9600;
	rs485.parity                       = PARITY_NONE;
	rs485.wordlength                   = WORDLENGTH_8;
	rs485.stopbits                     = STOPBITS_1;
	rs485.duplex                       = DUPLEX_HALF;
	rs485.duplex_shift_setting         = 0x1;
	rs485.error_count_overrun          = 0;
	rs485.error_count_parity           = 0;
	rs485.error_count_parity           = 0;
	rs485.read_callback_enabled        = false;
	rs485.error_count_callback_enabled = false;
	rs485.frame_readable_cb_frame_size = 0;

	// Modbus specific.
	rs485.modbus_slave_address          = MODBUS_DEFAULT_SLAVE_ADDRESS;
	rs485.modbus_master_request_timeout = MODBUS_DEFAULT_MASTER_REQUEST_TIMEOUT;

	// By default we use a 50/50 segmentation of rx/tx buffer
	rs485.buffer_size_rx = RS485_BUFFER_SIZE/2;

	rs485_init_buffer();
	rs485_init_hardware();
    timer_init();

	if(rs485.mode == MODE_MODBUS_SLAVE_RTU || rs485.mode == MODE_MODBUS_MASTER_RTU) {
		modbus_init(&rs485);
	}
}

void rs485_tick(void) {
	// We try to read the RX buffer in every tick:
	// 1. We may have data in the buffer (just by coincidence).
	//    In this case we can save the interrupt call overhead.
	// 2. The interrupt is only triggered if more then 16 bytes
	//    are in the RX buffer, so we always read the end of
	//    a big message or messages <16 bytes here.
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RX);
	NVIC_DisableIRQ((IRQn_Type)RS485_IRQ_RXA);
	bool new_data = false;
	while(!XMC_USIC_CH_RXFIFO_IsEmpty(RS485_USIC)) {
		new_data = true;
		// Instead of ringbuffer_add() we add the byte to the buffer
		// by hand.
		//
		// We need to save the low watermark calculation overhead.

		uint16_t new_end = *rs485_ringbuffer_rx_end + 1;

		if(new_end >= *rs485_ringbuffer_rx_size) {
			new_end = 0;
		}

		if(new_end == *rs485_ringbuffer_rx_start) {
			rs485.error_count_overrun++;

			// In the case of an overrun we read the byte and throw it away.
			volatile uint8_t __attribute__((unused)) _  = RS485_USIC->OUTR;
		} else {
			rs485_ringbuffer_rx_buffer[*rs485_ringbuffer_rx_end] = RS485_USIC->OUTR;
			*rs485_ringbuffer_rx_end = new_end;
		}
	}

	if(new_data) {
		TIMER_RESET();
	}
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RXA);
	NVIC_EnableIRQ((IRQn_Type)RS485_IRQ_RX);


	if(rs485.mode == MODE_MODBUS_SLAVE_RTU || rs485.mode == MODE_MODBUS_MASTER_RTU) {
		modbus_update_rtu_wire_state_machine(&rs485);
	}
}