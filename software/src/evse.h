/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * evse.h: EVSE implementation
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

#ifndef EVSE_H
#define EVSE_H

#include <stdint.h>
#include <stdbool.h>

#include "iec61851.h"

#define EVSE_CP_PWM_PERIOD    48000 // 1kHz
#define EVSE_MOTOR_PWM_PERIOD 4800  // 10kHz
#define EVSE_BOOST_MODE_US    4     // 1 us = 0.06A

#define EVSE_CONFIG_JUMPER_CURRENT_6A   0
#define EVSE_CONFIG_JUMPER_CURRENT_10A  1
#define EVSE_CONFIG_JUMPER_CURRENT_13A  2
#define EVSE_CONFIG_JUMPER_CURRENT_16A  3
#define EVSE_CONFIG_JUMPER_CURRENT_20A  4
#define EVSE_CONFIG_JUMPER_CURRENT_25A  5
#define EVSE_CONFIG_JUMPER_CURRENT_32A  6
#define EVSE_CONFIG_JUMPER_RESERVED     7
#define EVSE_CONFIG_JUMPER_UNCONFIGURED 8

#define EVSE_CONFIG_PAGE                1
#define EVSE_CONFIG_MAGIC_POS           0
#define EVSE_CONFIG_MANAGED_POS         1
#define EVSE_CONFIG_REL_SUM_POS         2
#define EVSE_CONFIG_SHUTDOWN_INPUT_POS  3
#define EVSE_CONFIG_MAGIC2_POS          4
#define EVSE_CONFIG_INPUT_POS           5
#define EVSE_CONFIG_OUTPUT_POS          6
#define EVSE_CONFIG_BUTTON_POS          7
#define EVSE_CONFIG_MAGIC3_POS          8
#define EVSE_CONFIG_BOOST_POS           9
#define EVSE_CONFIG_EV_WAKUEP_POS       10
#define EVSE_CONFIG_MAGIC4_POS          11
#define EVSE_CONFIG_REL_IMPORT_POS      12
#define EVSE_CONFIG_REL_EXPORT_POS      13
#define EVSE_CONFIG_REL_EXPORT_POS      13
#define EVSE_CONFIG_MAGIC5_POS          14
#define EVSE_CONFIG_AUTOSWITCH_POS      15
#define EVSE_CONFIG_SLOT_DEFAULT_POS    48

typedef struct {
	uint16_t current[18];
	uint8_t active_clear[18];
	uint32_t magic;
} __attribute__((__packed__)) EVSEChargingSlotDefault;

#define EVSE_CONFIG_MAGIC               0x34567892
#define EVSE_CONFIG_MAGIC2              0x45678923
#define EVSE_CONFIG_MAGIC3              0x56789234
#define EVSE_CONFIG_MAGIC4              0x67892345
#define EVSE_CONFIG_MAGIC5              0x78923456
#define EVSE_CONFIG_SLOT_MAGIC          0x62870616

#define EVSE_STORAGE_PAGES              16

typedef struct {
    uint32_t startup_time;

	uint8_t config_jumper_current;

	bool has_lock_switch;
	bool calibration_error;
	bool legacy_managed;

	uint32_t factory_reset_time;

	uint32_t last_contactor_switch;

	uint8_t shutdown_input_configuration;
	uint8_t input_configuration;
	uint8_t output_configuration;

	uint32_t charging_time;

	bool ev_wakeup_enabled;
	bool control_pilot_disconnect;

	uint32_t communication_watchdog_time;

	uint32_t contactor_turn_off_time;

	bool boost_mode_enabled;

	uint8_t storage[EVSE_STORAGE_PAGES][64];
} EVSE;

extern EVSE evse;

bool evse_is_cp_connected(void);
bool evse_is_shutdown(void);
void evse_save_config(void);
void evse_set_output(const float cp_duty_cycle, const bool contactor);
uint16_t evse_get_cp_duty_cycle(void);
void evse_set_cp_duty_cycle(const float duty_cycle);
void evse_init(void);
void evse_tick(void);

#endif
