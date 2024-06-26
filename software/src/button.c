/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * button.c: EVSE button driver
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

#include "button.h"

#include "configs/config_button.h"

#include "bricklib2/hal/system_timer/system_timer.h"

#include "button.h"
#include "evse.h"
#include "led.h"
#include "communication.h"
#include "charging_slot.h"
#include "iec61851.h"

#include <string.h>

#define BUTTON_DEBOUNCE_STANDARD 100 // ms
#define BUTTON_DEBOUNCE_LONG     2000 // ms

Button button;

void button_init(void) {
	// Save button configuration (it is persistent and set before button_init is called)
	uint8_t button_conf_tmp = button.configuration;

	memset(&button, 0, sizeof(Button));

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_BUTTON_PIN, &pin_config_input);

	button.configuration = button_conf_tmp;
	button.debounce_time = BUTTON_DEBOUNCE_STANDARD;

	button.boot_press_start = system_timer_get_ms();
}

void button_tick(void) {
	const bool value = XMC_GPIO_GetInput(EVSE_BUTTON_PIN);

	// Implement boot press time for recovery mode
	if(!button.boot_done) {
		if(!value) {
			button.boot_press_time = system_timer_get_ms() - button.boot_press_start;

			// Only accept boot press times > 2s
			if(button.boot_press_time < 2000) {
				button.boot_press_time = 0;
			}
			button.boot_done = true;
		}
	}

	if(value != button.last_value) {
		button.last_value = value;
		button.last_change_time = system_timer_get_ms();

		// DEBOUNCE button state will be overwritten after debounce time
		if(!value) {
			button.state = BUTTON_STATE_RELEASED_DEBOUNCE;
			// We always see a button release as a state change that turns the LED on (until standby)
			led_set_on(false);
		} else {
			button.state = BUTTON_STATE_PRESSED_DEBOUNCE;
		}
	}

	if(button.last_change_time != 0 && system_timer_is_time_elapsed_ms(button.last_change_time, button.debounce_time)) {
		button.last_change_time = 0;
		button.debounce_time    = BUTTON_DEBOUNCE_STANDARD;
		if(!value) {
			button.state = BUTTON_STATE_RELEASED;
			button.release_time = system_timer_get_ms();
		} else {
			button.state = BUTTON_STATE_PRESSED;
			button.press_time = system_timer_get_ms();

			if((iec61851.state == IEC61851_STATE_B) && (charging_slot.max_current[CHARGING_SLOT_BUTTON] == 0)) { // B1
				// If start charging through button pressed is enabled
				if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_START_CHARGING) {
					// Simulate start-charging press in web interface
					charging_slot_start_charging_by_button();

					// In the case that we start the charging through a button press,
					// we increase the button debounce to 2 seconds if the button is configured to also stop charging,
					// to make sure to never start the charge and stop it again immediately.
					if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING) {
						button.debounce_time = BUTTON_DEBOUNCE_LONG;
					}
				}
			} else if(iec61851.state == IEC61851_STATE_B) { // B2
				if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING) {
					// Disallow charging bybutton charging slot
					charging_slot_stop_charging_by_button();

					// In the case that we stop the charging through a button press,
					// we increase the button debounce to 2 seconds if the button is configured to also start charging,
					// to make sure to never stop the charge and start it again immediately.
					if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_START_CHARGING) {
						button.debounce_time = BUTTON_DEBOUNCE_LONG;
					}
				}
			} else if(iec61851.state == IEC61851_STATE_C) {
				if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING) {
					// Disallow charging bybutton charging slot
					charging_slot_stop_charging_by_button();

					// In the case that we stop the charging through a button press,
					// we increase the button debounce to 2 seconds if the button is configured to also start charging,
					// to make sure to never stop the charge and start it again immediately.
					if(button.configuration & EVSE_V2_BUTTON_CONFIGURATION_START_CHARGING) {
						button.debounce_time = BUTTON_DEBOUNCE_LONG;
					}
				}
			}
		}
	}

	// As long as the button is pressed the LED stays off
	if(button.state == BUTTON_STATE_PRESSED) {
		led_set_off();
	}
}