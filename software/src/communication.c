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
#include "bricklib2/warp/meter.h"
#include "bricklib2/warp/rs485.h"
#include "bricklib2/warp/contactor_check.h"

#include "configs/config_evse.h"
#include "configs/config_contactor_check.h"
#include "evse.h"
#include "adc.h"
#include "iec61851.h"
#include "led.h"
#include "lock.h"
#include "button.h"
#include "dc_fault.h"
#include "charging_slot.h"
#include "hardware_version.h"
#include "phase_control.h"
#include "tmp1075n.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	const uint8_t length = ((TFPMessageHeader*)message)->length;

	// Restart communication watchdog timer.
	evse.communication_watchdog_time = system_timer_get_ms();

	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE:                             return length != sizeof(GetState)                         ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION:            return length != sizeof(GetHardwareConfiguration)         ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE:                   return length != sizeof(GetLowLevelState)                 ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_low_level_state(message, response);
		case FID_SET_CHARGING_SLOT:                     return length != sizeof(SetChargingSlot)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_slot(message);
		case FID_SET_CHARGING_SLOT_MAX_CURRENT:         return length != sizeof(SetChargingSlotMaxCurrent)        ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_slot_max_current(message);
		case FID_SET_CHARGING_SLOT_ACTIVE:              return length != sizeof(SetChargingSlotActive)            ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_slot_active(message);
		case FID_SET_CHARGING_SLOT_CLEAR_ON_DISCONNECT: return length != sizeof(SetChargingSlotClearOnDisconnect) ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_slot_clear_on_disconnect(message);
		case FID_GET_CHARGING_SLOT:                     return length != sizeof(GetChargingSlot)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_charging_slot(message, response);
		case FID_GET_ALL_CHARGING_SLOTS:                return length != sizeof(GetAllChargingSlots)              ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_all_charging_slots(message, response);
		case FID_SET_CHARGING_SLOT_DEFAULT:             return length != sizeof(SetChargingSlotDefault)           ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_slot_default(message);
		case FID_GET_CHARGING_SLOT_DEFAULT:             return length != sizeof(GetChargingSlotDefault)           ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_charging_slot_default(message, response);
		case FID_GET_ENERGY_METER_VALUES:               return length != sizeof(GetEnergyMeterValues)             ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_energy_meter_values(message, response);
		case FID_GET_ALL_ENERGY_METER_VALUES_LOW_LEVEL: return length != sizeof(GetAllEnergyMeterValuesLowLevel)  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_all_energy_meter_values_low_level(message, response);
		case FID_GET_ENERGY_METER_ERRORS:               return length != sizeof(GetEnergyMeterErrors)             ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_energy_meter_errors(message, response);
		case FID_RESET_ENERGY_METER_RELATIVE_ENERGY:    return length != sizeof(ResetEnergyMeterRelativeEnergy)   ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : reset_energy_meter_relative_energy(message);
		case FID_RESET_DC_FAULT_CURRENT_STATE:          return length != sizeof(ResetDCFaultCurrentState)         ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : reset_dc_fault_current_state(message);
		case FID_SET_GPIO_CONFIGURATION:                return length != sizeof(SetGPIOConfiguration)             ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_gpio_configuration(message);
		case FID_GET_GPIO_CONFIGURATION:                return length != sizeof(GetGPIOConfiguration)             ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_gpio_configuration(message, response);
		case FID_GET_DATA_STORAGE:                      return length != sizeof(GetDataStorage)                   ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_data_storage(message, response);
		case FID_SET_DATA_STORAGE:                      return length != sizeof(SetDataStorage)                   ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_data_storage(message);
		case FID_GET_INDICATOR_LED:                     return length != sizeof(GetIndicatorLED)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_indicator_led(message, response);
		case FID_SET_INDICATOR_LED:                     return length != sizeof(SetIndicatorLED)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_indicator_led(message, response);
		case FID_SET_BUTTON_CONFIGURATION:              return length != sizeof(SetButtonConfiguration)           ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_button_configuration(message);
		case FID_GET_BUTTON_CONFIGURATION:              return length != sizeof(GetButtonConfiguration)           ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_button_configuration(message, response);
		case FID_GET_BUTTON_STATE:                      return length != sizeof(GetButtonState)                   ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_button_state(message, response);
		case FID_SET_EV_WAKEUP:                         return length != sizeof(SetEVWakeup)                      ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_ev_wakeup(message);
		case FID_GET_EV_WAKUEP:                         return length != sizeof(GetEVWakuep)                      ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_ev_wakuep(message, response);
		case FID_SET_CONTROL_PILOT_DISCONNECT:          return length != sizeof(SetControlPilotDisconnect)        ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_control_pilot_disconnect(message, response);
		case FID_GET_CONTROL_PILOT_DISCONNECT:          return length != sizeof(GetControlPilotDisconnect)        ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_control_pilot_disconnect(message, response);
		case FID_GET_ALL_DATA_1:                        return length != sizeof(GetAllData1)                      ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_all_data_1(message, response);
		case FID_GET_ALL_DATA_2:                        return length != sizeof(GetAllData2)                      ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_all_data_2(message, response);
		case FID_FACTORY_RESET:                         return length != sizeof(FactoryReset)                     ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : factory_reset(message);
		case FID_GET_BUTTON_PRESS_BOOT_TIME:            return length != sizeof(GetButtonPressBootTime)           ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_button_press_boot_time(message, response);
		case FID_SET_BOOST_MODE:                        return length != sizeof(SetBoostMode)                     ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_boost_mode(message);
		case FID_GET_BOOST_MODE:                        return length != sizeof(GetBoostMode)                     ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_boost_mode(message, response);
		case FID_TRIGGER_DC_FAULT_TEST:                 return length != sizeof(TriggerDCFaultTest)               ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : trigger_dc_fault_test(message, response);
		case FID_SET_GP_OUTPUT:                         return length != sizeof(SetGPOutput)                      ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_gp_output(message);
		case FID_GET_TEMPERATURE:                       return length != sizeof(GetTemperature)                   ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_temperature(message, response);
		case FID_SET_PHASE_CONTROL:                     return length != sizeof(SetPhaseControl)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_phase_control(message);
		case FID_GET_PHASE_CONTROL:                     return length != sizeof(GetPhaseControl)                  ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_phase_control(message, response);
		case FID_SET_PHASE_AUTO_SWITCH:                 return length != sizeof(SetPhaseAutoSwitch)               ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_phase_auto_switch(message);
		case FID_GET_PHASE_AUTO_SWITCH:                 return length != sizeof(GetPhaseAutoSwitch)               ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_phase_auto_switch(message, response);
		case FID_SET_PHASES_CONNECTED:                  return length != sizeof(SetPhasesConnected)               ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_phases_connected(message);
		case FID_GET_PHASES_CONNECTED:                  return length != sizeof(GetPhasesConnected)               ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_phases_connected(message, response);
		case FID_SET_CHARGING_PROTOCOL:                 return length != sizeof(SetChargingProtocol)              ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : set_charging_protocol(message);
		case FID_GET_CHARGING_PROTOCOL:                 return length != sizeof(GetChargingProtocol)              ? HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER : get_charging_protocol(message, response);
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

	if(response->error_state != 0) {
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
	response->evse_version          = hardware_version.is_v2 ? 20 : (hardware_version.is_v3 ? 30 : (hardware_version.is_v4 ? 40 : 0));

	if(!meter.each_value_read_once) {
		response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE;
	} else {
		switch(meter.type) {
			case METER_TYPE_UNKNOWN:       response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE; break;
			case METER_TYPE_UNSUPPORTED:   response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE; break;
			case METER_TYPE_SDM630:        response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_SDM630; break;
			case METER_TYPE_SDM72V2:       response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_SDM72V2; break;
			case METER_TYPE_SDM72CTM:      response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_SDM72CTM; break;
			case METER_TYPE_SDM630MCTV2:   response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_SDM630MCTV2; break;
			case METER_TYPE_DSZ15DZMOD:    response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_DSZ15DZMOD; break;
			case METER_TYPE_DEM4A:         response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_DEM4A; break;
			case METER_TYPE_DMED341MID7ER: response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_DMED341MID7ER; break;
			case METER_TYPE_DSZ16DZE:      response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_DSZ16DZE; break;
			case METER_TYPE_WM3M4C:        response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_WM3M4C; break;
			default:                       response->energy_meter_type = EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE; break;
		}
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

bool get_bit(const uint32_t value, uint8_t const bit_position) {
	return (bool)(value & (1 << bit_position));
}

BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response) {
	response->header.length             = sizeof(GetLowLevelState_Response);
	response->led_state                 = led.state;
	response->cp_pwm_duty_cycle         = evse_get_cp_duty_cycle();
	response->adc_values[0]             = adc[0].result[0];
	response->adc_values[1]             = adc[1].result[0];
	response->adc_values[2]             = adc[0].result[1];
	response->adc_values[3]             = adc[1].result[1];
	response->adc_values[4]             = adc[2].result[0];
	response->adc_values[5]             = adc[3].result[0];
	response->adc_values[6]             = adc[4].result[0];
	response->voltages[0]               = adc[0].result_mv[0];
	response->voltages[1]               = adc[1].result_mv[0];
	response->voltages[2]               = adc[0].result_mv[1];
	response->voltages[3]               = adc[1].result_mv[1];
	response->voltages[4]               = adc[2].result_mv[0];
	response->voltages[5]               = adc[3].result_mv[0];
	response->voltages[6]               = adc[4].result_mv[0];
	response->resistances[0]            = adc_result.cp_pe_resistance;
	response->resistances[1]            = adc_result.pp_pe_resistance;
	response->uptime                    = system_timer_get_ms();
	response->time_since_state_change   = response->uptime - iec61851.last_state_change;
	response->time_since_dc_fault_check = response->uptime - dc_fault.last_run_time;

	const uint32_t port0 = XMC_GPIO_PORT0->IN;
	const uint32_t port1 = XMC_GPIO_PORT1->IN;
	const uint32_t port2 = XMC_GPIO_PORT2->IN;
	const uint32_t port4 = XMC_GPIO_PORT4->IN;

	if(hardware_version.is_v2) {
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
	} else { // v3 or v4
		response->gpio[0] = (get_bit(port0, 0)   << 0) | //  0: DC X30
		                    (get_bit(port0, 1)   << 1) | //  1: DC X6
		                    (get_bit(port0, 3)   << 2) | //  2: DC Error
		                    (get_bit(port0, 5)   << 3) | //  3: DC Test
		                    (get_bit(port0, 6)   << 4) | //  4: Status LED
		                    (get_bit(port0, 12)  << 5) | //  5: Switch
		                    (get_bit(port1, 0)   << 6) | //  6: LED R
		                    (get_bit(port1, 2)   << 7);  //  7: LED B

		response->gpio[1] = (get_bit(port1, 3)   << 0) | //  8: LED G
		                    (get_bit(port1, 4)   << 1) | //  9: CP PWM
		                    (get_bit(port1, 5)   << 2) | // 10: Contactor 1
		                    (get_bit(port1, 6)   << 3) | // 11: Contactor 0
		                    (get_bit(port2, 6)   << 4) | // 12: Contactor 1 FB
		                    (get_bit(port2, 7)   << 5) | // 13: Contactor 0 FB
		                    (get_bit(port2, 8)   << 6) | // 14: PE Check
		                    (get_bit(port2, 9)   << 7);  // 15: Config Jumper 1

		response->gpio[2] = (get_bit(port4, 4)   << 0) | // 16: CP Disconnect
		                    (get_bit(port4, 5)   << 1) | // 17: Config Jumper 0
		                    (get_bit(port4, 6)   << 2) | // 18: Enable
		                    (get_bit(port4, 7)   << 3);  // 19: Version Detection
		if(hardware_version.is_v4) {
			response->gpio[2] |= (get_bit(port1, 1)   << 4) |  // 20: Lock In1
			                     (get_bit(port0, 2)   << 5) |  // 21: Lock In2
			                     (get_bit(port0, 9)   << 6);   // 22: Lock Feedback
		}
	}

	response->car_stopped_charging = evse.car_stopped_charging;

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

	// If button is pressed we don't allow to change the max current in the button slot
	if(!((data->slot == CHARGING_SLOT_BUTTON) && (button.state == BUTTON_STATE_PRESSED) && (button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING))) {
		charging_slot.max_current[data->slot] = data->max_current;
	}
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

	// If button is pressed we don't allow to change the max current in the button slot
	if(!((data->slot == CHARGING_SLOT_BUTTON) && (button.state == BUTTON_STATE_PRESSED) && (button.configuration & EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING))) {
		charging_slot.max_current[data->slot] = data->max_current;
	}

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
	response->power            = meter_register_set.total_system_power.f;
	response->current[0]       = meter_register_set.current[0].f;
	response->current[1]       = meter_register_set.current[1].f;
	response->current[2]       = meter_register_set.current[2].f;
	response->phases_active[0] = (((meter_register_set.current[0].f > 0.3f) & meter.phases_connected[0]) << 0) |
	                             (((meter_register_set.current[1].f > 0.3f) & meter.phases_connected[1]) << 1) |
	                             (((meter_register_set.current[2].f > 0.3f) & meter.phases_connected[2]) << 2);
	response->phases_connected[0] = (meter.phases_connected[0] << 0) |
	                                (meter.phases_connected[1] << 1) |
	                                (meter.phases_connected[2] << 2);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_energy_meter_values_low_level(const GetAllEnergyMeterValuesLowLevel *data, GetAllEnergyMeterValuesLowLevel_Response *response) {
	static uint32_t packet_payload_index = 0;

	response->header.length = sizeof(GetAllEnergyMeterValuesLowLevel_Response);

	const uint8_t packet_length = 60;
	const uint16_t max_end = 88*sizeof(float);
	const uint16_t start = packet_payload_index * packet_length;
	const uint16_t end = MIN(start + packet_length, max_end);
	const uint16_t copy_num = end-start;
	uint8_t *copy_from = (uint8_t*)&meter_register_set;

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
	meter.reset_energy_meter = true;
	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse reset_dc_fault_current_state(const ResetDCFaultCurrentState *data) {
	// TODO: Only do dc fault reset if currently no calibration running
	if(data->password == 0xDC42FA23) {
		// Set back dc fault state to normal
		dc_fault.state = DC_FAULT_NORMAL_CONDITION;

		// Completely reset dc fault calibration state
		dc_fault.calibration_running = false;
		dc_fault_calibration_reset();

		// Immediately start a new dc fault calibration (to go back to error state in case of a defective dc fault sensor)
		dc_fault.calibration_start = true;
		led_set_on(false);
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_gpio_configuration(const SetGPIOConfiguration *data) {
	evse.shutdown_input_configuration = data->shutdown_input_configuration;
	evse.input_configuration          = data->input_configuration;
	evse.output_configuration         = data->output_configuration;

	if(hardware_version.is_v2) {
		// n-channel mosfet (signals inverted)
		if(evse.output_configuration == EVSE_V2_OUTPUT_CONNECTED_TO_GROUND) {
			XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
		} else if(evse.output_configuration == EVSE_V2_OUTPUT_HIGH_IMPEDANCE) {
			XMC_GPIO_SetOutputLow(EVSE_OUTPUT_GP_PIN);
		}
	}

	evse_save_config(); // Save shutdown input, input and output config

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
	response->color_h       = led.set_h;
	response->color_s       = led.set_s;
	response->color_v       = led.set_v;
	if((led.api_duration == 0) || system_timer_is_time_elapsed_ms(led.api_start, led.api_duration)) {
		response->duration  = 0;
	} else {
		response->duration  = led.api_duration - ((uint32_t)(system_timer_get_ms() - led.api_start));
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_indicator_led(const SetIndicatorLED *data, SetIndicatorLED_Response *response) {
	if((data->indication >= 256) && (data->indication != 1001) && (data->indication != 1002) && (data->indication != 1003) && ((data->indication < 2001) || (data->indication > 2010))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(SetIndicatorLED_Response);

	led.set_h = data->color_h;
	led.set_s = data->color_s;
	led.set_v = data->color_v;

	if(hardware_version.is_v2) {
		led.h = LED_HUE_BLUE;
		led.s = 255;
		led.v = 255;
	} else if(hardware_version.is_v3 || hardware_version.is_v4) {
		if(data->color_v == 0) {
			led.s = 255;
			led.v = 255;
			if(data->indication == 1001) {
				led.h = LED_HUE_OK;
			} else if(data->indication == 1002) {
				led.h = LED_HUE_ERROR;
			} else if(data->indication == 1003) {
				led.h = LED_HUE_YELLOW;
			} else if((data->indication > 2000) && (data->indication < 2011)) {
				led.h = LED_HUE_ERROR;
			} else {
				led.h = LED_HUE_STANDARD;
			}
		} else {
			led.h = data->color_h;
			led.s = data->color_s;
			led.v = data->color_v;
		}
	}

	// If the state and indication stays the same we just update the duration
	// This way the animation does not become choppy
	if((led.state == LED_STATE_API) && (led.api_indication == data->indication)) {

		led.api_duration = data->duration;
		led.api_start    = system_timer_get_ms();
		response->status = 0;
		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	// Otherwise we reset the current animation and start the new one
	if((led.state == LED_STATE_OFF) || (led.state == LED_STATE_ON) || (led.state == LED_STATE_API) || (led.state == LED_STATE_BREATHING)) {
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

	evse_save_config(); // Save button config

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
	response->button_pressed      = (button.state == BUTTON_STATE_PRESSED) || (button.state == BUTTON_STATE_PRESSED_DEBOUNCE);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_ev_wakeup(const SetEVWakeup *data) {
	// If ev wakeup is enabled we adhere to IEC 61851 Annex A.5.3
	// Information on difficulties encountered with some legacy EVs
	// for wake-up after a long period of inactivity.
	evse.ev_wakeup_enabled = data->ev_wakeup_enabled;
	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_ev_wakuep(const GetEVWakuep *data, GetEVWakuep_Response *response) {
	response->header.length = sizeof(GetEVWakuep_Response);
	response->ev_wakeup_enabled = evse.ev_wakeup_enabled;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_control_pilot_disconnect(const SetControlPilotDisconnect *data, SetControlPilotDisconnect_Response *response) {
	if(data->control_pilot_disconnect) {
		// Only allow cp disconnect in state A or B.
		if(((iec61851.state == IEC61851_STATE_A) || (iec61851.state == IEC61851_STATE_B)) && (phase_control.progress_state == 0)) {
			// Only if contactor is currently not active
			if(XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN)) { // active low
				evse_cp_disconnect();

				evse.control_pilot_disconnect = data->control_pilot_disconnect;

				// Don't do EV wakeup if CP disconnect is controlled externally
				iec61851_reset_ev_wakeup();
			}
		}
	} else {
		// If we are currently waking up the EV we don't allow CP disconnect to be turned off again from external
		if(!iec61851.currently_beeing_woken_up && (phase_control.progress_state == 0)) {
			evse_cp_connect();

			evse.control_pilot_disconnect = data->control_pilot_disconnect;
		}
	}

	response->header.length               = sizeof(SetControlPilotDisconnect_Response);
	response->is_control_pilot_disconnect = evse.control_pilot_disconnect;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_control_pilot_disconnect(const GetControlPilotDisconnect *data, GetControlPilotDisconnect_Response *response) {
	response->header.length = sizeof(GetControlPilotDisconnect_Response);
	response->control_pilot_disconnect = evse.control_pilot_disconnect;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response) {
	response->header.length = sizeof(GetAllData1_Response);
	TFPMessageFull parts;

	get_state(NULL, (GetState_Response*)&parts);
	memcpy(&response->iec61851_state, parts.data, sizeof(GetState_Response) - sizeof(TFPMessageHeader));

	get_hardware_configuration(NULL, (GetHardwareConfiguration_Response*)&parts);
	memcpy(&response->jumper_configuration, parts.data, sizeof(GetHardwareConfiguration_Response) - sizeof(TFPMessageHeader));

	get_energy_meter_values(NULL, (GetEnergyMeterValues_Response*)&parts);
	memcpy(&response->power, parts.data, sizeof(GetEnergyMeterValues_Response) - sizeof(TFPMessageHeader));

	get_energy_meter_errors(NULL, (GetEnergyMeterErrors_Response*)&parts);
	memcpy(&response->error_count[0], parts.data, sizeof(GetEnergyMeterErrors_Response) - sizeof(TFPMessageHeader));

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_data_2(const GetAllData2 *data, GetAllData2_Response *response) {
	response->header.length = sizeof(GetAllData2_Response);
	TFPMessageFull parts;

	get_gpio_configuration(NULL, (GetGPIOConfiguration_Response*)&parts);
	memcpy(&response->shutdown_input_configuration, parts.data, sizeof(GetGPIOConfiguration_Response) - sizeof(TFPMessageHeader));

	get_indicator_led(NULL, (GetIndicatorLED_Response*)&parts);
	memcpy(&response->indication, parts.data, sizeof(GetIndicatorLED_Response) - sizeof(TFPMessageHeader));

	get_button_configuration(NULL, (GetButtonConfiguration_Response*)&parts);
	memcpy(&response->button_configuration, parts.data, sizeof(GetButtonConfiguration_Response) - sizeof(TFPMessageHeader));

	get_button_state(NULL, (GetButtonState_Response*)&parts);
	memcpy(&response->button_press_time, parts.data, sizeof(GetButtonState_Response) - sizeof(TFPMessageHeader));

	get_ev_wakuep(NULL, (GetEVWakuep_Response*)&parts);
	memcpy(&response->ev_wakeup_enabled, parts.data, sizeof(GetEVWakuep_Response) - sizeof(TFPMessageHeader));

	get_control_pilot_disconnect(NULL, (GetControlPilotDisconnect_Response*)&parts);
	memcpy(&response->control_pilot_disconnect, parts.data, sizeof(GetControlPilotDisconnect_Response) - sizeof(TFPMessageHeader));

	get_boost_mode(NULL, (GetBoostMode_Response*)&parts);
	memcpy(&response->boost_mode_enabled, parts.data, sizeof(GetBoostMode_Response) - sizeof(TFPMessageHeader));

	get_temperature(NULL, (GetTemperature_Response*)&parts);
	memcpy(&response->temperature, parts.data, sizeof(GetTemperature_Response) - sizeof(TFPMessageHeader));

	get_phase_control(NULL, (GetPhaseControl_Response*)&parts);
	memcpy(&response->phases_current, parts.data, sizeof(GetPhaseControl_Response) - sizeof(TFPMessageHeader));

	get_phase_auto_switch(NULL, (GetPhaseAutoSwitch_Response*)&parts);
	memcpy(&response->phase_auto_switch_enabled, parts.data, sizeof(GetPhaseAutoSwitch_Response) - sizeof(TFPMessageHeader));

	get_phases_connected(NULL, (GetPhasesConnected_Response*)&parts);
	memcpy(&response->phases_connected, parts.data, sizeof(GetPhasesConnected_Response) - sizeof(TFPMessageHeader));

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse factory_reset(const FactoryReset *data) {
	if(data->password == 0x2342FACD) {
		evse.factory_reset_time = system_timer_get_ms();
		return HANDLE_MESSAGE_RESPONSE_EMPTY;
	}

	return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
}

BootloaderHandleMessageResponse get_button_press_boot_time(const GetButtonPressBootTime *data, GetButtonPressBootTime_Response *response) {
	response->header.length = sizeof(GetButtonPressBootTime_Response);
	if(button.boot_done) {
		response->button_press_boot_time = button.boot_press_time;
	} else {
		response->button_press_boot_time = 0xFFFFFFFF;
	}

	if(data->reset) {
		button.boot_done       = true;
		button.boot_press_time = 0;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_boost_mode(const SetBoostMode *data) {
	evse.boost_mode_enabled = data->boost_mode_enabled;
	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_boost_mode(const GetBoostMode *data, GetBoostMode_Response *response) {
	response->header.length       = sizeof(GetBoostMode_Response);
	response->boost_mode_enabled = evse.boost_mode_enabled;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse trigger_dc_fault_test(const TriggerDCFaultTest *data, TriggerDCFaultTest_Response *response) {
	response->header.length = sizeof(TriggerDCFaultTest_Response);
	if(!dc_fault.calibration_running && (iec61851.state == IEC61851_STATE_A) && (adc_result.cp_pe_resistance > 0xFFFF) && XMC_GPIO_GetInput(EVSE_CONTACTOR_PIN)) {
		response->started = true;
		dc_fault.calibration_start = true;
	} else {
		response->started = false;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_gp_output(const SetGPOutput *data) {
	if(hardware_version.is_v2) {
		if(data->gp_output == EVSE_V2_OUTPUT_CONNECTED_TO_GROUND) {
			XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
		} else if(data->gp_output == EVSE_V2_OUTPUT_HIGH_IMPEDANCE) {
			XMC_GPIO_SetOutputLow(EVSE_OUTPUT_GP_PIN);
		}
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_temperature(const GetTemperature *data, GetTemperature_Response *response) {
	response->header.length = sizeof(GetTemperature_Response);
	if(hardware_version.is_v2) {
		response->temperature = 0;
	} else { // v3 or v4
		response->temperature = tmp1075n.temperature;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_phase_control(const SetPhaseControl *data) {
	if(!((data->phases == 1) || (data->phases == 3))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	// Ignore request if only one phase is connected
	if(phase_control.phases_connected == 1) {
		return HANDLE_MESSAGE_RESPONSE_EMPTY;
	}

	if(hardware_version.is_v3 || hardware_version.is_v4) {
		phase_control.requested = data->phases;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_phase_control(const GetPhaseControl *data, GetPhaseControl_Response *response) {
	response->header.length    = sizeof(GetPhaseControl_Response);

	response->phases_current   = phase_control.current;
	response->phases_requested = phase_control.requested;
	response->phases_state     = phase_control.progress_state;
	response->phases_info      = phase_control.info;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_phase_auto_switch(const SetPhaseAutoSwitch *data) {
	if(phase_control.autoswitch_enabled != data->phase_auto_switch_enabled) {
		phase_control.autoswitch_enabled = data->phase_auto_switch_enabled;
		evse_save_config();
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_phase_auto_switch(const GetPhaseAutoSwitch *data, GetPhaseAutoSwitch_Response *response) {
	response->header.length             = sizeof(GetPhaseAutoSwitch_Response);
	response->phase_auto_switch_enabled = phase_control.autoswitch_enabled;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_phases_connected(const SetPhasesConnected *data) {
	if((data->phases_connected != 1) && (data->phases_connected != 3)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(phase_control.phases_connected != data->phases_connected) {
		phase_control.phases_connected = data->phases_connected;

		phase_control.requested = data->phases_connected;
		if(hardware_version.is_v2) {
			// In V2 set current directly to requested, since we can't switch the phases physically
			phase_control.current = data->phases_connected;
		}
		evse_save_config();
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_phases_connected(const GetPhasesConnected *data, GetPhasesConnected_Response *response) {
	response->header.length    = sizeof(GetPhasesConnected_Response);
	response->phases_connected = phase_control.phases_connected;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_charging_protocol(const SetChargingProtocol *data) {
	if(hardware_version.is_v4) {
		iec61851.iso15118_active        = data->charging_protocol == EVSE_V2_CHARGING_PROTOCOL_ISO15118;
		iec61851.iso15118_cp_duty_cycle = data->cp_duty_cycle;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_protocol(const GetChargingProtocol *data, GetChargingProtocol_Response *response) {
	response->header.length = sizeof(GetChargingProtocol_Response);
	if(hardware_version.is_v4 && iec61851.iso15118_active) {
		response->charging_protocol = EVSE_V2_CHARGING_PROTOCOL_ISO15118;
		response->cp_duty_cycle     = iec61851.iso15118_cp_duty_cycle;
	} else {
		response->charging_protocol = EVSE_V2_CHARGING_PROTOCOL_IEC61851;
		response->cp_duty_cycle     = evse_get_cp_duty_cycle();
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


bool handle_energy_meter_values_callback(void) {
	static bool is_buffered = false;
	static EnergyMeterValues_Callback cb;

	if(!is_buffered) {
		if(meter.new_fast_value_callback) {
			get_energy_meter_values(NULL, (GetEnergyMeterValues_Response*)&cb);
			tfp_make_default_header(&cb.header, bootloader_get_uid(), sizeof(EnergyMeterValues_Callback), FID_CALLBACK_ENERGY_METER_VALUES);
			meter.new_fast_value_callback = false;
		} else {
			return false;
		}
	}

	if(bootloader_spitfp_is_send_possible(&bootloader_status.st)) {
		bootloader_spitfp_send_ack_and_message(&bootloader_status, (uint8_t*)&cb, sizeof(EnergyMeterValues_Callback));
		is_buffered = false;
		return true;
	} else {
		is_buffered = true;
	}

	return false;
}

void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}
