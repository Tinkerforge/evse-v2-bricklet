/* evse-v2-bricklet
 * Copyright (C) 2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.h: TFP protocol message handling
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

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"

// Default functions
BootloaderHandleMessageResponse handle_message(const void *data, void *response);
void communication_tick(void);
void communication_init(void);

// Constants

#define EVSE_V2_IEC61851_STATE_A 0
#define EVSE_V2_IEC61851_STATE_B 1
#define EVSE_V2_IEC61851_STATE_C 2
#define EVSE_V2_IEC61851_STATE_D 3
#define EVSE_V2_IEC61851_STATE_EF 4

#define EVSE_V2_LED_STATE_OFF 0
#define EVSE_V2_LED_STATE_ON 1
#define EVSE_V2_LED_STATE_BLINKING 2
#define EVSE_V2_LED_STATE_FLICKER 3
#define EVSE_V2_LED_STATE_BREATHING 4

#define EVSE_V2_CHARGER_STATE_NOT_CONNECTED 0
#define EVSE_V2_CHARGER_STATE_WAITING_FOR_CHARGE_RELEASE 1
#define EVSE_V2_CHARGER_STATE_READY_TO_CHARGE 2
#define EVSE_V2_CHARGER_STATE_CHARGING 3
#define EVSE_V2_CHARGER_STATE_ERROR 4

#define EVSE_V2_CONTACTOR_STATE_AC1_NLIVE_AC2_NLIVE 0
#define EVSE_V2_CONTACTOR_STATE_AC1_LIVE_AC2_NLIVE 1
#define EVSE_V2_CONTACTOR_STATE_AC1_NLIVE_AC2_LIVE 2
#define EVSE_V2_CONTACTOR_STATE_AC1_LIVE_AC2_LIVE 3

#define EVSE_V2_LOCK_STATE_INIT 0
#define EVSE_V2_LOCK_STATE_OPEN 1
#define EVSE_V2_LOCK_STATE_CLOSING 2
#define EVSE_V2_LOCK_STATE_CLOSE 3
#define EVSE_V2_LOCK_STATE_OPENING 4
#define EVSE_V2_LOCK_STATE_ERROR 5

#define EVSE_V2_ERROR_STATE_OK 0
#define EVSE_V2_ERROR_STATE_SWITCH 2
#define EVSE_V2_ERROR_STATE_DC_FAULT 3
#define EVSE_V2_ERROR_STATE_CONTACTOR 4
#define EVSE_V2_ERROR_STATE_COMMUNICATION 5

#define EVSE_V2_JUMPER_CONFIGURATION_6A 0
#define EVSE_V2_JUMPER_CONFIGURATION_10A 1
#define EVSE_V2_JUMPER_CONFIGURATION_13A 2
#define EVSE_V2_JUMPER_CONFIGURATION_16A 3
#define EVSE_V2_JUMPER_CONFIGURATION_20A 4
#define EVSE_V2_JUMPER_CONFIGURATION_25A 5
#define EVSE_V2_JUMPER_CONFIGURATION_32A 6
#define EVSE_V2_JUMPER_CONFIGURATION_SOFTWARE 7
#define EVSE_V2_JUMPER_CONFIGURATION_UNCONFIGURED 8

#define EVSE_V2_DC_FAULT_CURRENT_STATE_NORMAL_CONDITION 0
#define EVSE_V2_DC_FAULT_CURRENT_STATE_6_MA_ERROR 1
#define EVSE_V2_DC_FAULT_CURRENT_STATE_SYSTEM_ERROR 2
#define EVSE_V2_DC_FAULT_CURRENT_STATE_UNKNOWN_ERROR 3
#define EVSE_V2_DC_FAULT_CURRENT_STATE_CALIBRATION_ERROR 4

#define EVSE_V2_SHUTDOWN_INPUT_IGNORED 0
#define EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_OPEN 1
#define EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_CLOSE 2

#define EVSE_V2_OUTPUT_LOW 0
#define EVSE_V2_OUTPUT_HIGH 1

#define EVSE_V2_BUTTON_CONFIGURATION_DEACTIVATED 0
#define EVSE_V2_BUTTON_CONFIGURATION_START_CHARGING 1
#define EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING 2
#define EVSE_V2_BUTTON_CONFIGURATION_START_AND_STOP_CHARGING 3

#define EVSE_V2_CONTROL_PILOT_DISCONNECTED 0
#define EVSE_V2_CONTROL_PILOT_CONNECTED 1
#define EVSE_V2_CONTROL_PILOT_AUTOMATIC 2

#define EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE 0
#define EVSE_V2_ENERGY_METER_TYPE_SDM72 1
#define EVSE_V2_ENERGY_METER_TYPE_SDM630 2
#define EVSE_V2_ENERGY_METER_TYPE_SDM72V2 3

#define EVSE_V2_BOOTLOADER_MODE_BOOTLOADER 0
#define EVSE_V2_BOOTLOADER_MODE_FIRMWARE 1
#define EVSE_V2_BOOTLOADER_MODE_BOOTLOADER_WAIT_FOR_REBOOT 2
#define EVSE_V2_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_REBOOT 3
#define EVSE_V2_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT 4

#define EVSE_V2_BOOTLOADER_STATUS_OK 0
#define EVSE_V2_BOOTLOADER_STATUS_INVALID_MODE 1
#define EVSE_V2_BOOTLOADER_STATUS_NO_CHANGE 2
#define EVSE_V2_BOOTLOADER_STATUS_ENTRY_FUNCTION_NOT_PRESENT 3
#define EVSE_V2_BOOTLOADER_STATUS_DEVICE_IDENTIFIER_INCORRECT 4
#define EVSE_V2_BOOTLOADER_STATUS_CRC_MISMATCH 5

#define EVSE_V2_STATUS_LED_CONFIG_OFF 0
#define EVSE_V2_STATUS_LED_CONFIG_ON 1
#define EVSE_V2_STATUS_LED_CONFIG_SHOW_HEARTBEAT 2
#define EVSE_V2_STATUS_LED_CONFIG_SHOW_STATUS 3

// Function and callback IDs and structs
#define FID_GET_STATE 1
#define FID_GET_HARDWARE_CONFIGURATION 2
#define FID_GET_LOW_LEVEL_STATE 3
#define FID_SET_CHARGING_SLOT 4
#define FID_SET_CHARGING_SLOT_MAX_CURRENT 5
#define FID_SET_CHARGING_SLOT_ACTIVE 6
#define FID_SET_CHARGING_SLOT_CLEAR_ON_DISCONNECT 7
#define FID_GET_CHARGING_SLOT 8
#define FID_GET_ALL_CHARGING_SLOTS 9
#define FID_SET_CHARGING_SLOT_DEFAULT 10
#define FID_GET_CHARGING_SLOT_DEFAULT 11
#define FID_GET_ENERGY_METER_VALUES 12
#define FID_GET_ALL_ENERGY_METER_VALUES_LOW_LEVEL 13
#define FID_GET_ENERGY_METER_ERRORS 14
#define FID_RESET_ENERGY_METER_RELATIVE_ENERGY 15
#define FID_RESET_DC_FAULT_CURRENT_STATE 16
#define FID_SET_GPIO_CONFIGURATION 17
#define FID_GET_GPIO_CONFIGURATION 18
#define FID_GET_DATA_STORAGE 19
#define FID_SET_DATA_STORAGE 20
#define FID_GET_INDICATOR_LED 21
#define FID_SET_INDICATOR_LED 22
#define FID_SET_BUTTON_CONFIGURATION 23
#define FID_GET_BUTTON_CONFIGURATION 24
#define FID_GET_BUTTON_STATE 25
#define FID_SET_CONTROL_PILOT_CONFIGURATION 26
#define FID_GET_CONTROL_PILOT_CONFIGURATION 27
#define FID_GET_ALL_DATA_1 28
#define FID_GET_ALL_DATA_2 29
#define FID_FACTORY_RESET 30


typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetState;

typedef struct {
	TFPMessageHeader header;
	uint8_t iec61851_state;
	uint8_t charger_state;
	uint8_t contactor_state;
	uint8_t contactor_error;
	uint16_t allowed_charging_current;
	uint8_t error_state;
	uint8_t lock_state;
	uint8_t dc_fault_current_state;
} __attribute__((__packed__)) GetState_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetHardwareConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint8_t jumper_configuration;
	bool has_lock_switch;
	uint8_t evse_version;
	uint8_t energy_meter_type;
} __attribute__((__packed__)) GetHardwareConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetLowLevelState;

typedef struct {
	TFPMessageHeader header;
	uint8_t led_state;
	uint16_t cp_pwm_duty_cycle;
	uint16_t adc_values[7];
	int16_t voltages[7];
	uint32_t resistances[2];
	uint8_t gpio[3];
	uint32_t charging_time;
	uint32_t time_since_state_change;
	uint32_t uptime;
} __attribute__((__packed__)) GetLowLevelState_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlot;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
} __attribute__((__packed__)) SetChargingSlotMaxCurrent;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	bool active;
} __attribute__((__packed__)) SetChargingSlotActive;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlotClearOnDisconnect;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
} __attribute__((__packed__)) GetChargingSlot;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) GetChargingSlot_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllChargingSlots;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current[20];
	uint8_t active_and_clear_on_disconnect[20];
} __attribute__((__packed__)) GetAllChargingSlots_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlotDefault;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
} __attribute__((__packed__)) GetChargingSlotDefault;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) GetChargingSlotDefault_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEnergyMeterValues;

typedef struct {
	TFPMessageHeader header;
	float power;
	float energy_relative;
	float energy_absolute;
	uint8_t phases_active[1];
	uint8_t phases_connected[1];
} __attribute__((__packed__)) GetEnergyMeterValues_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllEnergyMeterValuesLowLevel;

typedef struct {
	TFPMessageHeader header;
	uint16_t values_chunk_offset;
	float values_chunk_data[15];
} __attribute__((__packed__)) GetAllEnergyMeterValuesLowLevel_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEnergyMeterErrors;

typedef struct {
	TFPMessageHeader header;
	uint32_t error_count[6];
} __attribute__((__packed__)) GetEnergyMeterErrors_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) ResetEnergyMeterRelativeEnergy;

typedef struct {
	TFPMessageHeader header;
	uint32_t password;
} __attribute__((__packed__)) ResetDCFaultCurrentState;

typedef struct {
	TFPMessageHeader header;
	uint8_t shutdown_input_configuration;
	uint8_t input_configuration;
	uint8_t output_configuration;
} __attribute__((__packed__)) SetGPIOConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetGPIOConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint8_t shutdown_input_configuration;
	uint8_t input_configuration;
	uint8_t output_configuration;
} __attribute__((__packed__)) GetGPIOConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t page;
} __attribute__((__packed__)) GetDataStorage;

typedef struct {
	TFPMessageHeader header;
	uint8_t data[63];
} __attribute__((__packed__)) GetDataStorage_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t page;
	uint8_t data[63];
} __attribute__((__packed__)) SetDataStorage;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetIndicatorLED;

typedef struct {
	TFPMessageHeader header;
	int16_t indication;
	uint16_t duration;
} __attribute__((__packed__)) GetIndicatorLED_Response;

typedef struct {
	TFPMessageHeader header;
	int16_t indication;
	uint16_t duration;
} __attribute__((__packed__)) SetIndicatorLED;

typedef struct {
	TFPMessageHeader header;
	uint8_t status;
} __attribute__((__packed__)) SetIndicatorLED_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t button_configuration;
} __attribute__((__packed__)) SetButtonConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetButtonConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint8_t button_configuration;
} __attribute__((__packed__)) GetButtonConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetButtonState;

typedef struct {
	TFPMessageHeader header;
	uint32_t button_press_time;
	uint32_t button_release_time;
	bool button_pressed;
} __attribute__((__packed__)) GetButtonState_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t control_pilot;
} __attribute__((__packed__)) SetControlPilotConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetControlPilotConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint8_t control_pilot;
} __attribute__((__packed__)) GetControlPilotConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllData1;

typedef struct {
	TFPMessageHeader header;
	uint8_t iec61851_state;
	uint8_t charger_state;
	uint8_t contactor_state;
	uint8_t contactor_error;
	uint16_t allowed_charging_current;
	uint8_t error_state;
	uint8_t lock_state;
	uint8_t dc_fault_current_state;
	uint8_t jumper_configuration;
	bool has_lock_switch;
	uint8_t evse_version;
	uint8_t energy_meter_type;
	float power;
	float energy_relative;
	float energy_absolute;
	uint8_t phases_active[1];
	uint8_t phases_connected[1];
	uint32_t error_count[6];
} __attribute__((__packed__)) GetAllData1_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllData2;

typedef struct {
	TFPMessageHeader header;
	uint8_t shutdown_input_configuration;
	uint8_t input_configuration;
	uint8_t output_configuration;
	int16_t indication;
	uint16_t duration;
	uint8_t button_configuration;
	uint32_t button_press_time;
	uint32_t button_release_time;
	bool button_pressed;
	uint8_t control_pilot;
} __attribute__((__packed__)) GetAllData2_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t password;
} __attribute__((__packed__)) FactoryReset;


// Function prototypes
BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response);
BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response);
BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response);
BootloaderHandleMessageResponse set_charging_slot(const SetChargingSlot *data);
BootloaderHandleMessageResponse set_charging_slot_max_current(const SetChargingSlotMaxCurrent *data);
BootloaderHandleMessageResponse set_charging_slot_active(const SetChargingSlotActive *data);
BootloaderHandleMessageResponse set_charging_slot_clear_on_disconnect(const SetChargingSlotClearOnDisconnect *data);
BootloaderHandleMessageResponse get_charging_slot(const GetChargingSlot *data, GetChargingSlot_Response *response);
BootloaderHandleMessageResponse get_all_charging_slots(const GetAllChargingSlots *data, GetAllChargingSlots_Response *response);
BootloaderHandleMessageResponse set_charging_slot_default(const SetChargingSlotDefault *data);
BootloaderHandleMessageResponse get_charging_slot_default(const GetChargingSlotDefault *data, GetChargingSlotDefault_Response *response);
BootloaderHandleMessageResponse get_energy_meter_values(const GetEnergyMeterValues *data, GetEnergyMeterValues_Response *response);
BootloaderHandleMessageResponse get_all_energy_meter_values_low_level(const GetAllEnergyMeterValuesLowLevel *data, GetAllEnergyMeterValuesLowLevel_Response *response);
BootloaderHandleMessageResponse get_energy_meter_errors(const GetEnergyMeterErrors *data, GetEnergyMeterErrors_Response *response);
BootloaderHandleMessageResponse reset_energy_meter_relative_energy(const ResetEnergyMeterRelativeEnergy *data);
BootloaderHandleMessageResponse reset_dc_fault_current_state(const ResetDCFaultCurrentState *data);
BootloaderHandleMessageResponse set_gpio_configuration(const SetGPIOConfiguration *data);
BootloaderHandleMessageResponse get_gpio_configuration(const GetGPIOConfiguration *data, GetGPIOConfiguration_Response *response);
BootloaderHandleMessageResponse get_data_storage(const GetDataStorage *data, GetDataStorage_Response *response);
BootloaderHandleMessageResponse set_data_storage(const SetDataStorage *data);
BootloaderHandleMessageResponse get_indicator_led(const GetIndicatorLED *data, GetIndicatorLED_Response *response);
BootloaderHandleMessageResponse set_indicator_led(const SetIndicatorLED *data, SetIndicatorLED_Response *response);
BootloaderHandleMessageResponse set_button_configuration(const SetButtonConfiguration *data);
BootloaderHandleMessageResponse get_button_configuration(const GetButtonConfiguration *data, GetButtonConfiguration_Response *response);
BootloaderHandleMessageResponse get_button_state(const GetButtonState *data, GetButtonState_Response *response);
BootloaderHandleMessageResponse set_control_pilot_configuration(const SetControlPilotConfiguration *data);
BootloaderHandleMessageResponse get_control_pilot_configuration(const GetControlPilotConfiguration *data, GetControlPilotConfiguration_Response *response);
BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response);
BootloaderHandleMessageResponse get_all_data_2(const GetAllData2 *data, GetAllData2_Response *response);
BootloaderHandleMessageResponse factory_reset(const FactoryReset *data);

// Callbacks


#define COMMUNICATION_CALLBACK_TICK_WAIT_MS 1
#define COMMUNICATION_CALLBACK_HANDLER_NUM 0
#define COMMUNICATION_CALLBACK_LIST_INIT \


#endif
