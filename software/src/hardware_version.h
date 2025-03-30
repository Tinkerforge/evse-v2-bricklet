/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * hardware_version.h: Hardware version detection and support
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

#ifndef HARDWARE_VERSION_H
#define HARDWARE_VERSION_H

#include <stdint.h>
#include <stdbool.h>

#include "xmc_gpio.h"
typedef struct {
	bool is_v2;
	bool is_v3;
	bool is_v4;
} HardwareVersion;

typedef struct {
	XMC_GPIO_PORT_t *const port;
	uint8_t pin;
} HardwareVersionPortPin;

extern HardwareVersion hardware_version;

void hardware_version_init(void);
XMC_GPIO_PORT_t *const hardware_version_get_port(const uint8_t pin_num);
const uint8_t hardware_version_get_pin(const uint8_t pin_num);
#endif
