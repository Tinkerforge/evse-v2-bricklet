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
#include "sdm630.h"
#include "rs485.h"
#include "dc_fault.h"
#include "charging_slot.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION: return get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE: return get_low_level_state(message, response);
		case FID_SET_CHARGING_SLOT: return set_charging_slot(message);
		case FID_SET_CHARGING_SLOT_MAX_CURRENT: return set_charging_slot_max_current(message);
		case FID_SET_CHARGING_SLOT_ACTIVE: return set_charging_slot_active(message);
		case FID_SET_CHARGING_SLOT_CLEAR_ON_DISCONNECT: return set_charging_slot_clear_on_disconnect(message);
		case FID_GET_CHARGING_SLOT: return get_charging_slot(message, response);
		case FID_GET_ALL_CHARGING_SLOTS: return get_all_charging_slots(message, response);
		case FID_SET_CHARGING_SLOT_DEFAULT: return set_charging_slot_default(message);
		case FID_GET_CHARGING_SLOT_DEFAULT: return get_charging_slot_default(message, response);
		case FID_GET_ENERGY_METER_VALUES: return get_energy_meter_values(message, response);
		case FID_GET_ALL_ENERGY_METER_VALUES_LOW_LEVEL: return get_all_energy_meter_values_low_level(message, response);
		case FID_GET_ENERGY_METER_ERRORS: return get_energy_meter_errors(message, response);
		case FID_RESET_ENERGY_METER_RELATIVE_ENERGY: return reset_energy_meter_relative_energy(message);
		case FID_RESET_DC_FAULT_CURRENT_STATE: return reset_dc_fault_current_state(message);
		case FID_SET_GPIO_CONFIGURATION: return set_gpio_configuration(message);
		case FID_GET_GPIO_CONFIGURATION: return get_gpio_configuration(message, response);
		case FID_GET_DATA_STORAGE: return get_data_storage(message, response);
		case FID_SET_DATA_STORAGE: return set_data_storage(message);
		case FID_GET_INDICATOR_LED: return get_indicator_led(message, response);
		case FID_SET_INDICATOR_LED: return set_indicator_led(message, response);
		case FID_SET_BUTTON_CONFIGURATION: return set_button_configuration(message);
		case FID_GET_BUTTON_CONFIGURATION: return get_button_configuration(message, response);
		case FID_GET_BUTTON_STATE: return get_button_state(message, response);
		case FID_SET_CONTROL_PILOT_CONFIGURATION: return set_control_pilot_configuration(message);
		case FID_GET_CONTROL_PILOT_CONFIGURATION: return get_control_pilot_configuration(message, response);
		case FID_GET_ALL_DATA_1: return get_all_data_1(message, response);
		case FID_GET_ALL_DATA_2: return get_all_data_2(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response) {
	response->header.length            = sizeof(GetState_Response);
	response->iec61851_state           = iec61851.state;
	response->contactor_state          = contactor_check.state;
	response->contactor_error          = contactor_check.error;
	response->allowed_charging_current = iec61851_get_max_ma();
	response->error_state              = led.state == LED_STATE_BLINKING ? led.blink_num : 0;
	response->lock_state               = lock.state;

	if((iec61851.state == IEC61851_STATE_D) || (iec61851.state == IEC61851_STATE_EF)) {
		response->charger_state = EVSE_V2_CHARGER_STATE_ERROR;
	} else if(iec61851.state == IEC61851_STATE_C) {
		response->charger_state = EVSE_V2_CHARGER_STATE_CHARGING;
	} else if(iec61851.state == IEC61851_STATE_B) {
		if(charging_slot_get_max_current() == 0) {
			response->charger_state = EVSE_V2_CHARGER_STATE_WAITING_FOR_CHARGE_RELEASE;
		} else {
			response->charger_state = EVSE_V2_CHARGER_STATE_READY_TO_CHARGE;
		}
	} else { 
		response->charger_state = EVSE_V2_CHARGER_STATE_NOT_CONNECTED;
	}

	response->dc_fault_current_state   = dc_fault.state;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response) {
	response->header.length         = sizeof(GetHardwareConfiguration_Response);
	response->jumper_configuration  = evse.config_jumper_current;
	response->has_lock_switch       = evse.has_lock_switch;
	response->evse_version          = 20;
	response->energy_meter_type     = EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE;
	if(sdm630.available) {
		response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_SDM630;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool get_bit(const uint32_t value, uint8_t const bit_position) {
	return (bool)(value & (1 << bit_position));
}

BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response) {
	response->header.length           = sizeof(GetLowLevelState_Response);
	response->led_state               = led.state;
	response->cp_pwm_duty_cycle       = evse_get_cp_duty_cycle();
	response->adc_values[0]           = adc[0].result[0];
	response->adc_values[1]           = adc[1].result[0];
	response->adc_values[2]           = adc[0].result[1];
	response->adc_values[3]           = adc[1].result[1];
	response->adc_values[4]           = adc[2].result[0];
	response->adc_values[5]           = adc[3].result[0];
	response->adc_values[6]           = adc[4].result[0];
	response->voltages[0]             = adc[0].result_mv[0];
	response->voltages[1]             = adc[1].result_mv[0];
	response->voltages[2]             = adc[0].result_mv[1];
	response->voltages[3]             = adc[1].result_mv[1];
	response->voltages[4]             = adc[2].result_mv[0];
	response->voltages[5]             = adc[3].result_mv[0];
	response->voltages[6]             = adc[4].result_mv[0];
	response->resistances[0]          = adc_result.cp_pe_resistance;
	response->resistances[1]          = adc_result.pp_pe_resistance;
	response->uptime                  = system_timer_get_ms();
	response->time_since_state_change = response->uptime - iec61851.last_state_change;

	const uint32_t port0 = XMC_GPIO_PORT0->IN;
	const uint32_t port1 = XMC_GPIO_PORT1->IN;
	const uint32_t port2 = XMC_GPIO_PORT2->IN;
	const uint32_t port4 = XMC_GPIO_PORT4->IN;

	response->gpio[0] = (get_bit(port0, 0)  << 0) | //  0: Config Jumper 0
	                    (get_bit(port0, 1)  << 1) | //  1: Motor Fault
	                    (get_bit(port0, 3)  << 2) | //  2: DC Error
	                    (get_bit(port0, 5)  << 3) | //  3: Config Jumper 1
	                    (get_bit(port0, 8)  << 4) | //  4: DC Test
	                    (get_bit(port0, 9)  << 5) | //  5: Enable
	                    (get_bit(port0, 12) << 6) | //  6: Switch
	                    (get_bit(port1, 0)  << 7);  //  7: CP PWM

	response->gpio[1] = (get_bit(port1, 1)  << 0) | //  8: Input Motor Switch
	                    (get_bit(port1, 2)  << 1) | //  9: Relay (Contactor)
	                    (get_bit(port1, 3)  << 2) | // 10: GP Output
	                    (get_bit(port1, 4)  << 3) | // 11: CP Disconnect
	                    (get_bit(port1, 5)  << 4) | // 12: Motor Enable
	                    (get_bit(port1, 6)  << 5) | // 13: Motor Phase
	                    (get_bit(port2, 6)  << 6) | // 14: AC 1
	                    (get_bit(port2, 7)  << 7);  // 15: AC 2

	response->gpio[2] = (get_bit(port2, 9)  << 0) | // 16: GP Input
	                    (get_bit(port4, 4)  << 1) | // 17: DC X6
	                    (get_bit(port4, 5)  << 2) | // 18: DC X30
	                    (get_bit(port4, 6)  << 3);  // 19: LED

	if(evse.charging_time == 0) {
		response->charging_time = 0;
	} else {
		response->charging_time = system_timer_get_ms() - evse.charging_time;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_charging_slot(const SetChargingSlot *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.max_current[data->slot]         = data->max_current;
	charging_slot.active[data->slot]              = data->active;
	charging_slot.clear_on_disconnect[data->slot] = data->clear_on_disconnect;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_max_current(const SetChargingSlotMaxCurrent *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.max_current[data->slot] = data->max_current;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_active(const SetChargingSlotActive *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.active[data->slot] = data->active;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_clear_on_disconnect(const SetChargingSlotClearOnDisconnect *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.clear_on_disconnect[data->slot] = data->clear_on_disconnect;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_slot(const GetChargingSlot *data, GetChargingSlot_Response *response) {
	if(data->slot >= CHARGING_SLOT_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length       = sizeof(GetChargingSlot_Response);
	response->max_current         = charging_slot.max_current[data->slot];
	response->active              = charging_slot.active[data->slot];
	response->clear_on_disconnect = charging_slot.clear_on_disconnect[data->slot];

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_charging_slots(const GetAllChargingSlots *data, GetAllChargingSlots_Response *response) {
	response->header.length = sizeof(GetAllChargingSlots_Response);
	for(uint8_t i = 0; i < CHARGING_SLOT_NUM; i++) {
		response->max_current[i]                    = charging_slot.max_current[i];
		response->active_and_clear_on_disconnect[i] = (charging_slot.active[i] << 0) | (charging_slot.clear_on_disconnect[i] << 1);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_charging_slot_default(const SetChargingSlotDefault *data) {
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	const uint8_t slot = data->slot - 2;

	charging_slot.max_current_default[slot]         = data->max_current;
	charging_slot.active_default[slot]              = data->active;
	charging_slot.clear_on_disconnect_default[slot] = data->clear_on_disconnect;

	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_slot_default(const GetChargingSlotDefault *data, GetChargingSlotDefault_Response *response) {
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	const uint8_t slot = data->slot - 2;

	response->header.length       = sizeof(GetChargingSlotDefault_Response);
	response->max_current         = charging_slot.max_current_default[slot];
	response->active              = charging_slot.active_default[slot];
	response->clear_on_disconnect = charging_slot.clear_on_disconnect_default[slot];

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_energy_meter_values(const GetEnergyMeterValues *data, GetEnergyMeterValues_Response *response) {
	response->header.length    = sizeof(GetEnergyMeterValues_Response);
	response->power            = sdm630_register_fast.power.f;
	response->energy_absolute  = sdm630_register_fast.absolute_energy.f;
	response->energy_relative  = sdm630_register_fast.absolute_energy.f - sdm630.relative_energy.f;
	response->phases_active[0] = (((sdm630_register_fast.current_per_phase[0].f > 0.3f) & sdm630.phases_connected[0]) << 0) |
	                             (((sdm630_register_fast.current_per_phase[1].f > 0.3f) & sdm630.phases_connected[1]) << 1) |
	                             (((sdm630_register_fast.current_per_phase[2].f > 0.3f) & sdm630.phases_connected[2]) << 2);
	response->phases_connected[0] = (sdm630.phases_connected[0] << 0) |
	                                (sdm630.phases_connected[1] << 1) |
	                                (sdm630.phases_connected[2] << 2);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_energy_meter_values_low_level(const GetAllEnergyMeterValuesLowLevel *data, GetAllEnergyMeterValuesLowLevel_Response *response) {
	static uint32_t packet_payload_index = 0;

	response->header.length = sizeof(GetAllEnergyMeterValuesLowLevel_Response);

	const uint8_t packet_length = 60;
	const uint16_t max_end = 84*sizeof(float);
	const uint16_t start = packet_payload_index * packet_length;
	const uint16_t end = MIN(start + packet_length, max_end);
	const uint16_t copy_num = end-start;
	uint8_t *copy_from = (uint8_t*)&sdm630_register;

	response->values_chunk_offset = start/4;
	memcpy(response->values_chunk_data, &copy_from[start], copy_num);

	if(end < max_end) {
		packet_payload_index++;
	} else {
		packet_payload_index = 0;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_energy_meter_errors(const GetEnergyMeterErrors *data, GetEnergyMeterErrors_Response *response) {
	response->header.length  = sizeof(GetEnergyMeterErrors_Response);
	response->error_count[0] = rs485.modbus_common_error_counters.timeout;
	response->error_count[1] = 0; // Global timeout. Currently global timeout triggers watchdog and EVSE will restart, so this will always be 0.
	response->error_count[2] = rs485.modbus_common_error_counters.illegal_function;
	response->error_count[3] = rs485.modbus_common_error_counters.illegal_data_address;
	response->error_count[4] = rs485.modbus_common_error_counters.illegal_data_value;
	response->error_count[5] = rs485.modbus_common_error_counters.slave_device_failure;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse reset_energy_meter_relative_energy(const ResetEnergyMeterRelativeEnergy *data) {
	sdm630.reset_energy_meter = true;
	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse reset_dc_fault_current_state(const ResetDCFaultCurrentState *data) {
	if(data->password == 0xDC42FA23) {
		dc_fault.state = DC_FAULT_NORMAL_CONDITION;
		led_set_on(false);
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_gpio_configuration(const SetGPIOConfiguration *data) {
	evse.shutdown_input_configuration = data->shutdown_input_configuration;
	evse.input_configuration          = data->input_configuration;
	evse.output_configuration         = data->output_configuration;

	// n-channel mosfet (signals inverted)
	if(evse.output_configuration == EVSE_V2_OUTPUT_LOW) {
		XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
	} else if(evse.output_configuration == EVSE_V2_OUTPUT_HIGH) {
		XMC_GPIO_SetOutputLow(EVSE_OUTPUT_GP_PIN);
	}

	evse_save_config(); // Save shutdown input config

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_gpio_configuration(const GetGPIOConfiguration *data, GetGPIOConfiguration_Response *response) {
	response->header.length                = sizeof(GetGPIOConfiguration_Response);
	response->shutdown_input_configuration = evse.shutdown_input_configuration;
	response->input_configuration          = evse.input_configuration;
	response->output_configuration         = evse.output_configuration;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_data_storage(const GetDataStorage *data, GetDataStorage_Response *response) {
	if(data->page >= EVSE_STORAGE_PAGES) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetDataStorage_Response);
	memcpy(response->data, evse.storage[data->page], 63);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_data_storage(const SetDataStorage *data) {
	if(data->page >= EVSE_STORAGE_PAGES) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	memcpy(evse.storage[data->page], data->data, 63);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_indicator_led(const GetIndicatorLED *data, GetIndicatorLED_Response *response) {
	response->header.length = sizeof(GetIndicatorLED_Response);
	response->indication    = led.api_indication;
	if((led.api_duration == 0) || system_timer_is_time_elapsed_ms(led.api_start, led.api_duration)) {
		response->duration  = 0;
	} else {
		response->duration  = led.api_duration - ((uint32_t)(system_timer_get_ms() - led.api_start));
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_indicator_led(const SetIndicatorLED *data, SetIndicatorLED_Response *response) {
	if((data->indication >= 256) && (data->indication != 1001) && (data->indication != 1002) && (data->indication != 1003)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(SetIndicatorLED_Response);

	// If the state and indication stays the same we just update the duration
	// This way the animation does not become choppy
	if((led.state == LED_STATE_API) && (led.api_indication == data->indication)) {
		led.api_duration = data->duration;
		led.api_start    = system_timer_get_ms();
		response->status = 0;
		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	// Otherwise we reset the current animation and start the new one
	if((led.state == LED_STATE_OFF) || (led.state == LED_STATE_ON) || (led.state == LED_STATE_API)) {
		response->status     = 0;

		led.api_ack_counter  = 0;
		led.api_ack_index    = 0;
		led.api_ack_time     = 0;

		led.api_nack_counter = 0;
		led.api_nack_index   = 0;
		led.api_nack_time    = 0;

		led.api_nag_counter  = 0;
		led.api_nag_index    = 0;
		led.api_nag_time     = 0;

		if(data->indication < 0) {
			// If LED state is currently LED_STATE_OFF or LED_STATE_ON we
			// leave the LED where it is (and don't restart the standby timer).
			// If LED state is currently in LED_STATE_API we
			// turn the LED on (which brings it in LED_STATE_ON) and restarts the standby timer.
			if(led.state == LED_STATE_API) {
				led_set_on(true);
			}
		} else {
			led.state          = LED_STATE_API;
			led.api_indication = data->indication;
			led.api_duration   = data->duration;
			led.api_start      = system_timer_get_ms();
		}
	} else {
		response->status = led.state;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_button_configuration(const SetButtonConfiguration *data) {
	if(data->button_configuration > EVSE_V2_BUTTON_CONFIGURATION_START_AND_STOP_CHARGING) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	button.configuration = data->button_configuration;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_button_configuration(const GetButtonConfiguration *data, GetButtonConfiguration_Response *response) {
	response->header.length        = sizeof(GetButtonConfiguration_Response);
	response->button_configuration = button.configuration;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_button_state(const GetButtonState *data, GetButtonState_Response *response) {
	response->header.length       = sizeof(GetButtonState_Response);
	response->button_press_time   = button.press_time;
	response->button_release_time = button.release_time;
	response->button_pressed      = button.state == BUTTON_STATE_PRESSED;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_control_pilot_configuration(const SetControlPilotConfiguration *data) {
	// TODO: Automatic mode not yet implemented. Currently Automatic = Connected
	switch(data->control_pilot) {
		case EVSE_V2_CONTROL_PILOT_DISCONNECTED: XMC_GPIO_SetOutputHigh(EVSE_CP_DISCONNECT_PIN); break;
		case EVSE_V2_CONTROL_PILOT_CONNECTED:    XMC_GPIO_SetOutputLow(EVSE_CP_DISCONNECT_PIN);  break;
		case EVSE_V2_CONTROL_PILOT_AUTOMATIC:    XMC_GPIO_SetOutputLow(EVSE_CP_DISCONNECT_PIN);  break;
		default: return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	evse.control_pilot = data->control_pilot;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_control_pilot_configuration(const GetControlPilotConfiguration *data, GetControlPilotConfiguration_Response *response) {
	response->header.length = sizeof(GetControlPilotConfiguration_Response);
	response->control_pilot = evse.control_pilot;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response) {
	response->header.length = sizeof(GetAllData1_Response);
	TFPMessageFull parts;

	get_state(NULL, (GetState_Response*)&parts);
	memcpy(&response->iec61851_state, parts.data, sizeof(GetState_Response) - sizeof(TFPMessageHeader));

	get_hardware_configuration(NULL, (GetHardwareConfiguration_Response*)&parts);
	memcpy(&response->jumper_configuration, parts.data, sizeof(GetHardwareConfiguration_Response) - sizeof(TFPMessageHeader));

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_data_2(const GetAllData2 *data, GetAllData2_Response *response) {
	response->header.length = sizeof(GetAllData2_Response);
	TFPMessageFull parts;

	get_energy_meter_values(NULL, (GetEnergyMeterValues_Response*)&parts);
	memcpy(&response->power, parts.data, sizeof(GetEnergyMeterValues_Response) - sizeof(TFPMessageHeader));

	get_energy_meter_errors(NULL, (GetEnergyMeterErrors_Response*)&parts);
	memcpy(&response->error_count[0], parts.data, sizeof(GetEnergyMeterErrors_Response) - sizeof(TFPMessageHeader));

	get_gpio_configuration(NULL, (GetGPIOConfiguration_Response*)&parts);
	memcpy(&response->shutdown_input_configuration, parts.data, sizeof(GetGPIOConfiguration_Response) - sizeof(TFPMessageHeader));

	get_indicator_led(NULL, (GetIndicatorLED_Response*)&parts);
	memcpy(&response->indication, parts.data, sizeof(GetIndicatorLED_Response) - sizeof(TFPMessageHeader));

	get_button_configuration(NULL, (GetButtonConfiguration_Response*)&parts);
	memcpy(&response->button_configuration, parts.data, sizeof(GetButtonConfiguration_Response) - sizeof(TFPMessageHeader));

	get_button_state(NULL, (GetButtonState_Response*)&parts);
	memcpy(&response->button_press_time, parts.data, sizeof(GetButtonState_Response) - sizeof(TFPMessageHeader));

	get_control_pilot_configuration(NULL, (GetControlPilotConfiguration_Response*)&parts);
	memcpy(&response->control_pilot, parts.data, sizeof(GetControlPilotConfiguration_Response) - sizeof(TFPMessageHeader));

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
