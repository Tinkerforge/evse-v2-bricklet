/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * main.c: Initialization for EVSE Bricklet 2.0
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

#include <stdio.h>
#include <stdbool.h>

#include "configs/config.h"

#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/warp/rs485.h"
#include "bricklib2/warp/sdm.h"
#include "communication.h"

#include "evse.h"
#include "iec61851.h"
#include "lock.h"
#include "contactor_check.h"
#include "led.h"
#include "button.h"
#include "adc.h"
#include "dc_fault.h"
#include "charging_slot.h"

int main(void) {
	logging_init();
	logd("Start EVSE Bricklet 2.0\n\r");

	communication_init();
	evse_init();
	charging_slot_init();
	iec61851_init();
	lock_init();
	contactor_check_init();
	led_init();
	button_init();
	adc_init();
	dc_fault_init();
	rs485_init();
	sdm_init();

	while(true) {
		bootloader_tick();
		communication_tick();
//		lock_tick();
		contactor_check_tick();
		led_tick();
		button_tick();
		adc_tick();
		evse_tick();
		dc_fault_tick();
		rs485_tick();
		sdm_tick();
		charging_slot_tick();
	}
}
