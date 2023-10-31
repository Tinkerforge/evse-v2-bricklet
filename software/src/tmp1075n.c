/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tmp1075n.c: Driver for TMP1075N temperature sensor
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

#include "tmp1075n.h"

#include "configs/config_tmp1075n.h"
#include "hardware_version.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/hal/i2c_fifo/i2c_fifo.h"
#include "bricklib2/logging/logging.h"

TMP1075N tmp1075n;

void tmp1075n_init(void) {
    // No TMP1075N in EVSE V2
    if(hardware_version.is_v2) {
        return;
    } 

    return;

    memset(&tmp1075n, 0, sizeof(TMP1075N));

    tmp1075n.i2c_fifo.baudrate         = TMP1075N_I2C_BAUDRATE;
    tmp1075n.i2c_fifo.address          = TMP1075N_I2C_ADDRESS;
    tmp1075n.i2c_fifo.i2c              = TMP1075N_I2C;

    tmp1075n.i2c_fifo.scl_port         = TMP1075N_SCL_PORT;
    tmp1075n.i2c_fifo.scl_pin          = TMP1075N_SCL_PIN;
    tmp1075n.i2c_fifo.scl_mode         = TMP1075N_SCL_PIN_MODE;
    tmp1075n.i2c_fifo.scl_input        = TMP1075N_SCL_INPUT;
    tmp1075n.i2c_fifo.scl_source       = TMP1075N_SCL_SOURCE;
    tmp1075n.i2c_fifo.scl_fifo_size    = TMP1075N_SCL_FIFO_SIZE;
    tmp1075n.i2c_fifo.scl_fifo_pointer = TMP1075N_SCL_FIFO_POINTER;

    tmp1075n.i2c_fifo.sda_port         = TMP1075N_SDA_PORT;
    tmp1075n.i2c_fifo.sda_pin          = TMP1075N_SDA_PIN;
    tmp1075n.i2c_fifo.sda_mode         = TMP1075N_SDA_PIN_MODE;
    tmp1075n.i2c_fifo.sda_input        = TMP1075N_SDA_INPUT;
    tmp1075n.i2c_fifo.sda_source       = TMP1075N_SDA_SOURCE;
    tmp1075n.i2c_fifo.sda_fifo_size    = TMP1075N_SDA_FIFO_SIZE;
    tmp1075n.i2c_fifo.sda_fifo_pointer = TMP1075N_SDA_FIFO_POINTER;

    i2c_fifo_init(&tmp1075n.i2c_fifo);
    tmp1075n.last_read = system_timer_get_ms();
}

void tmp1075n_tick(void) {
    // No TMP1075N in EVSE V2
    if(hardware_version.is_v2) {
        return;
    } 

    return;

    I2CFifoState state = i2c_fifo_next_state(&tmp1075n.i2c_fifo);

    // A read did not finish within 60 seconds
    if(system_timer_is_time_elapsed_ms(tmp1075n.last_read, 60*1000)) {
        loge("TMP1075N I2C timeout: %d\n\r", state);
        tmp1075n_init();
        return;
    }

    if(state & I2C_FIFO_STATE_ERROR) {
        loge("TMP1075N I2C error: %d\n\r", state);
        tmp1075n_init();
        return;
    }

    switch(state) {
        case I2C_FIFO_STATE_READ_REGISTER_READY: {
            uint8_t buffer[2];
            uint8_t length = i2c_fifo_read_fifo(&tmp1075n.i2c_fifo, buffer, 2);
            if(length != 2) {
                loge("TMP1075N I2C unexpected read length : %d\n\r", length);
                tmp1075n_init();
                break;
            }

            int16_t value = ((buffer[0] << 8) | buffer[1]) >> 4;
            // 12-bit to 16-bit two-complement
            if(value & (1 << 11)) {
                value = value | 0xF000;
            }
            tmp1075n.temperature = ((((int32_t)value) * 625) / 100);       

            tmp1075n.last_read = system_timer_get_ms();
            break;
        }

        case I2C_FIFO_STATE_WRITE_REGISTER_READY: {
            // Nothing to do here.
            break;
        }

        case I2C_FIFO_STATE_IDLE: {
            break; // Handled below
        }

        default: {
            // If we end up in a ready state that we don't handle, something went wrong
            if(state & I2C_FIFO_STATE_READY) {
                loge("TMP1075N I2C unrecognized ready state : %d\n\r", state);
                tmp1075n_init();
            }
            return;
        }
    }


    if((state == I2C_FIFO_STATE_IDLE) || (state & I2C_FIFO_STATE_READY)) {
        // Read temperature once per 500ms
        if(system_timer_is_time_elapsed_ms(tmp1075n.last_read, 500)) {
            i2c_fifo_read_direct(&tmp1075n.i2c_fifo, 2, false);
        }
    }

}
