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

#define EVSE_CP_PWM_PERIOD    48000 // 1kHz
#define EVSE_MOTOR_PWM_PERIOD 4800  // 10kHz

#define EVSE_CONFIG_JUMPER_CURRENT_6A   0
#define EVSE_CONFIG_JUMPER_CURRENT_10A  1
#define EVSE_CONFIG_JUMPER_CURRENT_13A  2
#define EVSE_CONFIG_JUMPER_CURRENT_16A  3
#define EVSE_CONFIG_JUMPER_CURRENT_20A  4
#define EVSE_CONFIG_JUMPER_CURRENT_25A  5
#define EVSE_CONFIG_JUMPER_CURRENT_32A  6
#define EVSE_CONFIG_JUMPER_SOFTWARE     7
#define EVSE_CONFIG_JUMPER_UNCONFIGURED 8

#define EVSE_CONFIG_PAGE                1
#define EVSE_CONFIG_MAGIC_POS           0
#define EVSE_CONFIG_MANAGED_POS         1
#define EVSE_CONFIG_REL_ENERGY_POS      2
#define EVSE_CONFIG_SHUTDOWN_INPUT_POS  3

#define EVSE_CONFIG_MAGIC               0x34567892

#define EVSE_STORAGE_PAGES              16

typedef struct {
    uint32_t startup_time;

	uint8_t config_jumper_current;
	bool has_lock_switch;

	uint16_t config_jumper_current_software;

	uint16_t max_current_configured;
	bool calibration_error;
	bool charging_autostart;

	uint32_t last_contactor_switch;

	bool managed;
	uint16_t max_managed_current;

	uint8_t shutdown_input_configuration;
	uint8_t input_configuration;
	uint8_t output_configuration;

	uint32_t time_since_cp_pwm_change;

	uint8_t storage[EVSE_STORAGE_PAGES][64];
} EVSE;

extern EVSE evse;

bool evse_is_shutdown(void);
void evse_save_config(void);
void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor);
uint16_t evse_get_cp_duty_cycle(void);
void evse_set_cp_duty_cycle(const uint16_t duty_cycle);
void evse_init(void);
void evse_tick(void);

#endif