/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
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

#include "communication.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/utility/util_definitions.h"

#include "configs/config_evse.h"
#include "configs/config_contactor_check.h"
#include "evse.h"
#include "adc.h"
#include "iec61851.h"
#include "led.h"
#include "contactor_check.h"
#include "lock.h"
#include "button.h"
#include "sdm72dm.h"
#include "rs485.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION: return get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE: return get_low_level_state(message, response);
		case FID_SET_MAX_CHARGING_CURRENT: return set_max_charging_current(message);
		case FID_GET_MAX_CHARGING_CURRENT: return get_max_charging_current(message, response);
		case FID_START_CHARGING: return start_charging(message);
		case FID_STOP_CHARGING: return stop_charging(message);
		case FID_SET_CHARGING_AUTOSTART: return set_charging_autostart(message);
		case FID_GET_CHARGING_AUTOSTART: return get_charging_autostart(message, response);
		case FID_GET_ENERGY_METER_VALUES: return get_energy_meter_values(message, response);
		case FID_GET_ENERGY_METER_STATE: return get_energy_meter_state(message, response);
		case FID_RESET_ENERGY_METER: return reset_energy_meter(message);
		case FID_GET_DC_FAULT_CURRENT_STATE: return get_dc_fault_current_state(message, response);
		case FID_RESET_DC_FAULT_CURRENT: return reset_dc_fault_current(message);
		case FID_SET_GPIO_CONFIGURATION: return set_gpio_configuration(message);
		case FID_GET_GPIO_CONFIGURATION: return get_gpio_configuration(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response) {
	response->header.length            = sizeof(GetState_Response);
	response->iec61851_state           = iec61851.state;
	response->contactor_state          = contactor_check.state;
	response->contactor_error          = contactor_check.error;
	if((button.state == BUTTON_STATE_PRESSED) || (iec61851.state >= IEC61851_STATE_D)) { // button is pressed or error state
		response->charge_release       = EVSE_V2_CHARGE_RELEASE_DEACTIVATED;
	} else if(!button.was_pressed && evse.charging_autostart) { // button was not pressed and autostart on
		response->charge_release       = EVSE_V2_CHARGE_RELEASE_AUTOMATIC;
	} else { // button was pressed or autostart off
		response->charge_release       = EVSE_V2_CHARGE_RELEASE_MANUAL;
	}
	response->allowed_charging_current = iec61851_get_max_ma();
	response->error_state              = led.state == LED_STATE_BLINKING ? led.blink_num : 0;
	response->lock_state               = lock.state;
	response->uptime                   = system_timer_get_ms();
	response->time_since_state_change  = response->uptime - iec61851.last_state_change;

	if((iec61851.state == IEC61851_STATE_D) || (iec61851.state == IEC61851_STATE_EF)) {
		response->vehicle_state = EVSE_V2_VEHICLE_STATE_ERROR;
	} else if(iec61851.state == IEC61851_STATE_C) {
		response->vehicle_state = EVSE_V2_VEHICLE_STATE_CHARGING;
	} else if(iec61851.state == IEC61851_STATE_B) {
		response->vehicle_state = EVSE_V2_VEHICLE_STATE_CONNECTED;
	} else { 
		// For state A we may be not connected or connected with autostart disabled.
		// We check this by looking at the CP/PE resistance. We expect at least 10000 ohm if a vehicle is not connected.
		if(adc_result.cp_pe_resistance > 10000) {
			response->vehicle_state = EVSE_V2_VEHICLE_STATE_NOT_CONNECTED;
		} else {
			response->vehicle_state = EVSE_V2_VEHICLE_STATE_CONNECTED;
		}
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response) {
	response->header.length        = sizeof(GetHardwareConfiguration_Response);
	response->jumper_configuration = evse.config_jumper_current;
	response->has_lock_switch      = evse.has_lock_switch;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response) {
	response->header.length          = sizeof(GetLowLevelState_Response);
	response->led_state              = led.state;
	response->cp_pwm_duty_cycle      = (48000 - ccu4_pwm_get_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER))/48;
	response->adc_values[0]          = adc[0].result;
	response->adc_values[1]          = adc[1].result;
	response->adc_values[2]          = adc[2].result;
	response->adc_values[3]          = adc[3].result;
	response->adc_values[4]          = adc[4].result;
	response->voltages[0]            = adc[0].result_mv;
	response->voltages[1]            = adc[1].result_mv;
	response->voltages[2]            = adc[2].result_mv;
	response->voltages[3]            = adc[3].result_mv;
	response->voltages[4]            = adc[4].result_mv;
	response->resistances[0]         = adc_result.cp_pe_resistance;
	response->resistances[1]         = adc_result.pp_pe_resistance;
	response->gpio[0]                = 0; // TODO
	response->gpio[1]                = 0; // TODO
	response->gpio[2]                = 0; // TODO

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_max_charging_current(const SetMaxChargingCurrent *data) {
	// Use a minimum of 6A and a maximum of 32A.
	evse.max_current_configured = BETWEEN(6000, data->max_current, 32000);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_max_charging_current(const GetMaxChargingCurrent *data, GetMaxChargingCurrent_Response *response) {
	response->header.length              = sizeof(GetMaxChargingCurrent_Response);
	response->max_current_configured     = evse.max_current_configured;
	response->max_current_outgoing_cable = iec61851_get_ma_from_pp_resistance();
	response->max_current_incoming_cable = iec61851_get_ma_from_jumper();

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse start_charging(const StartCharging *data) {
	// Starting a new charge is the same as "releasing" a button press

	// If autostart is disabled we only accept a start chargin request if a car is currently connected
	if(((iec61851.state == IEC61851_STATE_A) && !evse.charging_autostart && (adc_result.cp_pe_resistance < 10000)) || 
	   evse.charging_autostart) {
		button.was_pressed = false;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse stop_charging(const StopCharging *data) {
	// Stopping the charging is the same as pressing the button
	button.was_pressed = true;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_autostart(const SetChargingAutostart *data) {
	evse.charging_autostart = data->autostart;

	// If autostart is disabled and there is not currently a car charging
	// we set "was_pressed" to make sure that the user needs to call start_charging before
	// the car starts charging.
	if(!evse.charging_autostart && (iec61851.state != IEC61851_STATE_C)) {
		button.was_pressed = true;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_autostart(const GetChargingAutostart *data, GetChargingAutostart_Response *response) {
	response->header.length = sizeof(GetChargingAutostart_Response);
	response->autostart     = evse.charging_autostart;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_energy_meter_values(const GetEnergyMeterValues *data, GetEnergyMeterValues_Response *response) {
	response->header.length   = sizeof(GetEnergyMeterValues_Response);
	response->power           = (uint32_t)sdm72dm.power.f;
	response->energy_absolute = (uint32_t)(sdm72dm.energy_absolute.f*1000.0f);
	response->energy_relative = (uint32_t)(sdm72dm.energy_relative.f*1000.0f);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_energy_meter_state(const GetEnergyMeterState *data, GetEnergyMeterState_Response *response) {
	response->header.length  = sizeof(GetEnergyMeterState_Response);
	response->available      = sdm72dm.available;
	response->error_count[0] = rs485.modbus_common_error_counters.timeout;
	response->error_count[1] = 0; // global timeout TODO
	response->error_count[2] = rs485.modbus_common_error_counters.illegal_function;
	response->error_count[3] = rs485.modbus_common_error_counters.illegal_data_address;
	response->error_count[4] = rs485.modbus_common_error_counters.illegal_data_value;
	response->error_count[5] = rs485.modbus_common_error_counters.slave_device_failure;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse reset_energy_meter(const ResetEnergyMeter *data) {
	sdm72dm.reset_energy_meter = true;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_dc_fault_current_state(const GetDCFaultCurrentState *data, GetDCFaultCurrentState_Response *response) {
	response->header.length = sizeof(GetDCFaultCurrentState_Response);

	// TODO

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse reset_dc_fault_current(const ResetDCFaultCurrent *data) {
	// TODO

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_gpio_configuration(const SetGPIOConfiguration *data) {
	// TODO

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_gpio_configuration(const GetGPIOConfiguration *data, GetGPIOConfiguration_Response *response) {
	response->header.length        = sizeof(GetGPIOConfiguration_Response);
	response->input_configuration  = 0; // TODO
	response->output_configuration = 0; // TODO

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
