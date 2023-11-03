/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * hardware_version.c: software/src/button.h
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

#include "hardware_version.h"

#include "configs/config_hardware_version.h"
#include "hardware_version.h"

#include "configs/config_dc_fault.h"
#include "configs/config_evse.h"

#include "xmc_gpio.h"

static const HardwareVersionPortPin hardware_version_v2[] = {
	{DC_FAULT_V2_X6_PIN},
	{DC_FAULT_V2_X30_PIN},
	{DC_FAULT_V2_ERR_PIN},
	{DC_FAULT_V2_TST_PIN},
	{EVSE_V2_CP_PWM_PIN},
	{EVSE_V2_CP_DISCONNECT_PIN},
	{EVSE_V2_INPUT_GP_PIN},
	{EVSE_V2_OUTPUT_GP_PIN},
	{EVSE_V2_SHUTDOWN_PIN},
	{EVSE_V2_CONFIG_JUMPER_PIN0},
	{EVSE_V2_CONFIG_JUMPER_PIN1},
	{EVSE_V2_CONTACTOR_PIN},
	{EVSE_V2_PHASE_SWITCH_PIN}
};

static const HardwareVersionPortPin hardware_version_v3[] = {
	{DC_FAULT_V3_X6_PIN},
	{DC_FAULT_V3_X30_PIN},
	{DC_FAULT_V3_ERR_PIN},
	{DC_FAULT_V3_TST_PIN},
	{EVSE_V3_CP_PWM_PIN},
	{EVSE_V3_CP_DISCONNECT_PIN},
	{EVSE_V3_INPUT_GP_PIN},
	{EVSE_V3_OUTPUT_GP_PIN},
	{EVSE_V3_SHUTDOWN_PIN},
	{EVSE_V3_CONFIG_JUMPER_PIN0},
	{EVSE_V3_CONFIG_JUMPER_PIN1},
	{EVSE_V3_CONTACTOR_PIN},
	{EVSE_V3_PHASE_SWITCH_PIN}
};

HardwareVersion hardware_version;

void hardware_version_init(void) {
	memset(&hardware_version, 0, sizeof(HardwareVersion));

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(HARDWARE_VERSION_DETECTION, &pin_config_input);

	hardware_version.is_v2 =  XMC_GPIO_GetInput(HARDWARE_VERSION_DETECTION); // EVSE V2 = version detection pin high
	hardware_version.is_v3 = !XMC_GPIO_GetInput(HARDWARE_VERSION_DETECTION); // EVSE V3 = version detection pin low
}

XMC_GPIO_PORT_t *const hardware_version_get_port(const uint8_t pin_num) {
	if(hardware_version.is_v2) {
		return hardware_version_v2[pin_num].port;
	} else if(hardware_version.is_v3) {
		return hardware_version_v3[pin_num].port;
	}

	return NULL;
}

const uint8_t hardware_version_get_pin(const uint8_t pin_num) {
	if(hardware_version.is_v2) {
		return hardware_version_v2[pin_num].pin;
	} else if(hardware_version.is_v3) {
		return hardware_version_v3[pin_num].pin;
	}

	return 0;
}