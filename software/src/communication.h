/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
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
#define EVSE_V2_DC_FAULT_CURRENT_STATE_6_MA_DC_ERROR 1
#define EVSE_V2_DC_FAULT_CURRENT_STATE_SYSTEM_ERROR 2
#define EVSE_V2_DC_FAULT_CURRENT_STATE_UNKNOWN_ERROR 3
#define EVSE_V2_DC_FAULT_CURRENT_STATE_CALIBRATION_ERROR 4
#define EVSE_V2_DC_FAULT_CURRENT_STATE_20_MA_AC_ERROR 5
#define EVSE_V2_DC_FAULT_CURRENT_STATE_6_MA_AC_AND_20_MA_AC_ERROR 6

#define EVSE_V2_SHUTDOWN_INPUT_IGNORED 0
#define EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_OPEN 1
#define EVSE_V2_SHUTDOWN_INPUT_SHUTDOWN_ON_CLOSE 2
#define EVSE_V2_SHUTDOWN_INPUT_4200_WATT_ON_OPEN 3
#define EVSE_V2_SHUTDOWN_INPUT_4200_WATT_ON_CLOSE 4

#define EVSE_V2_OUTPUT_CONNECTED_TO_GROUND 0
#define EVSE_V2_OUTPUT_HIGH_IMPEDANCE 1

#define EVSE_V2_BUTTON_CONFIGURATION_DEACTIVATED 0
#define EVSE_V2_BUTTON_CONFIGURATION_START_CHARGING 1
#define EVSE_V2_BUTTON_CONFIGURATION_STOP_CHARGING 2
#define EVSE_V2_BUTTON_CONFIGURATION_START_AND_STOP_CHARGING 3
#define EVSE_V2_BUTTON_CONFIGURATION_ENUMERATE 4

#define EVSE_V2_CONTROL_PILOT_DISCONNECTED 0
#define EVSE_V2_CONTROL_PILOT_CONNECTED 1
#define EVSE_V2_CONTROL_PILOT_AUTOMATIC 2

#define EVSE_V2_ENERGY_METER_TYPE_NOT_AVAILABLE 0
#define EVSE_V2_ENERGY_METER_TYPE_SDM72 1
#define EVSE_V2_ENERGY_METER_TYPE_SDM630 2
#define EVSE_V2_ENERGY_METER_TYPE_SDM72V2 3
#define EVSE_V2_ENERGY_METER_TYPE_SDM72CTM 4
#define EVSE_V2_ENERGY_METER_TYPE_SDM630MCTV2 5
#define EVSE_V2_ENERGY_METER_TYPE_DSZ15DZMOD 6
#define EVSE_V2_ENERGY_METER_TYPE_DEM4A 7
#define EVSE_V2_ENERGY_METER_TYPE_DMED341MID7ER 8
#define EVSE_V2_ENERGY_METER_TYPE_DSZ16DZE 9
#define EVSE_V2_ENERGY_METER_TYPE_WM3M4C 10

#define EVSE_V2_INPUT_UNCONFIGURED 0
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_0A 1
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_6A 2
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_8A 3
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_10A 4
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_13A 5
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_16A 6
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_20A 7
#define EVSE_V2_INPUT_ACTIVE_LOW_MAX_25A 8
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_0A 9
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_6A 10
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_8A 11
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_10A 12
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_13A 13
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_16A 14
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_20A 15
#define EVSE_V2_INPUT_ACTIVE_HIGH_MAX_25A 16

#define EVSE_V2_CHARGING_PROTOCOL_IEC61851 0
#define EVSE_V2_CHARGING_PROTOCOL_ISO15118 1

#define EVSE_V2_EICHRECHT_STATE_OK 0
#define EVSE_V2_EICHRECHT_STATE_NOT_ALL_INFO_SET 1
#define EVSE_V2_EICHRECHT_STATE_BUSY 2
#define EVSE_V2_EICHRECHT_STATE_NOT_SUPPORTED 3

#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_RFID_NONE 0
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_RFID_PLAIN 1
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_RFID_RELATED 2
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_RFID_PSK 3
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_NONE 4
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_RS 5
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_AUTH 6
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_RS_TLS 7
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_AUTH_TLS 8
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_CACHE 9
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_WHITELIST 10
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_OCPP_CERTIFIED 11
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_ISO15118_NONE 12
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_ISO15118_PNC 13
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_PLMN_NONE 14
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_PLMN_RING 15
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_PLMN_SMS 16
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_FLAG_NOT_SET 17

#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_NONE 0
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_DENIED 1
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_UNDEFINED 2
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_ISO14443 3
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_ISO15693 4
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_EMAID 5
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_EVCCID 6
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_EVCOID 7
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_ISO7812 8
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_CARD_TXN_NR 9
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_CENTRAL 10
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_CENTRAL_1 11
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_CENTRAL_2 12
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_LOCAL 13
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_LOCAL_1 14
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_LOCAL_2 15
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_PHONE_NUMBER 16
#define EVSE_V2_EICHRECHT_USER_ASSIGNMENT_IDENTIFICATION_TYPE_KEY_CODE 17

#define EVSE_V2_EICHRECHT_CHARGE_POINT_IDENTIFICATION_TYPE_EVSEID 0
#define EVSE_V2_EICHRECHT_CHARGE_POINT_IDENTIFICATION_TYPE_CBIDC 1

#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_NOT_INITIALISED 0
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_IDLE 1
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_SIGNATURE_IN_PROGRESS 2
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_SIGNATURE_OK 15
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_DATE_TIME 128
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_CHECKSUM_ERROR 129
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_COMMAND 130
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_STATE 131
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_MEASUREMENT 132
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_TEST_MODE_ERROR 133
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_VERIFY_STATE_ERROR 243
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_SIGNATURE_STATE_ERROR 244
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_KEYPAIR_GENERATION 245
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_SHA_FAILED 246
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INIT_FAILED 247
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_DATA_NOT_LOCKED 248
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_CONFIG_NOT_LOCKED 249
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_VERIFY_ERROR 250
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_PUBLIC_KEY_ERROR 251
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_MESSAGE_FORMAT 252
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_INVALID_MESSAGE_SIZE 253
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_SIGNATURE_ERROR 254
#define EVSE_V2_EICHRECHT_SIGNATURE_STATUS_UNDEFINED_ERROR 255

#define EVSE_V2_EICHRECHT_SIGNATURE_FORMAT_ASN1 0
#define EVSE_V2_EICHRECHT_SIGNATURE_FORMAT_BASE64 1

#define EVSE_V2_EICHRECHT_MEASUREMENT_STATUS_IDLE 0
#define EVSE_V2_EICHRECHT_MEASUREMENT_STATUS_ACTIVE 1
#define EVSE_V2_EICHRECHT_MEASUREMENT_STATUS_ACTIVE_AFTER_POWER_FAILURE 2
#define EVSE_V2_EICHRECHT_MEASUREMENT_STATUS_ACTIVE_AFTER_RESET 3
#define EVSE_V2_EICHRECHT_MEASUREMENT_STATUS_INVALID_DATE_TIME 128

#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_BEGIN 'B'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_END 'E'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_INTERMEDIATE 'C'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_EXCEPTION 'X'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_TARIFF_CHANGE 'T'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_SUSPENDED 'S'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_END_WITH_BEGIN 'r'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_FISCAL_READING 'f'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_HOLD_COMMAND 'h'
#define EVSE_V2_EICHRECHT_TRANSACTION_COMMAND_LAST_CHARGE_READING 'i'

#define EVSE_V2_CP_RECONNECT_TIME_DEFAULT 0
#define EVSE_V2_CP_RECONNECT_TIME_15_SECONDS 1
#define EVSE_V2_CP_RECONNECT_TIME_20_SECONDS 2
#define EVSE_V2_CP_RECONNECT_TIME_25_SECONDS 3
#define EVSE_V2_CP_RECONNECT_TIME_30_SECONDS 4
#define EVSE_V2_CP_RECONNECT_TIME_35_SECONDS 5
#define EVSE_V2_CP_RECONNECT_TIME_40_SECONDS 6
#define EVSE_V2_CP_RECONNECT_TIME_45_SECONDS 7
#define EVSE_V2_CP_RECONNECT_TIME_50_SECONDS 8
#define EVSE_V2_CP_RECONNECT_TIME_55_SECONDS 9
#define EVSE_V2_CP_RECONNECT_TIME_60_SECONDS 10
#define EVSE_V2_CP_RECONNECT_TIME_65_SECONDS 11
#define EVSE_V2_CP_RECONNECT_TIME_70_SECONDS 12
#define EVSE_V2_CP_RECONNECT_TIME_75_SECONDS 13
#define EVSE_V2_CP_RECONNECT_TIME_80_SECONDS 14
#define EVSE_V2_CP_RECONNECT_TIME_85_SECONDS 15
#define EVSE_V2_CP_RECONNECT_TIME_90_SECONDS 16
#define EVSE_V2_CP_RECONNECT_TIME_95_SECONDS 17
#define EVSE_V2_CP_RECONNECT_TIME_100_SECONDS 18
#define EVSE_V2_CP_RECONNECT_TIME_105_SECONDS 19
#define EVSE_V2_CP_RECONNECT_TIME_110_SECONDS 20
#define EVSE_V2_CP_RECONNECT_TIME_115_SECONDS 21
#define EVSE_V2_CP_RECONNECT_TIME_120_SECONDS 22

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
#define FID_SET_EV_WAKEUP 26
#define FID_GET_EV_WAKUEP 27
#define FID_SET_CONTROL_PILOT_DISCONNECT 28
#define FID_GET_CONTROL_PILOT_DISCONNECT 29
#define FID_GET_ALL_DATA_1 30
#define FID_GET_ALL_DATA_2 31
#define FID_FACTORY_RESET 32
#define FID_GET_BUTTON_PRESS_BOOT_TIME 33
#define FID_SET_BOOST_MODE 34
#define FID_GET_BOOST_MODE 35
#define FID_TRIGGER_DC_FAULT_TEST 36
#define FID_SET_GP_OUTPUT 37
#define FID_GET_TEMPERATURE 38
#define FID_SET_PHASE_CONTROL 39
#define FID_GET_PHASE_CONTROL 40
#define FID_SET_PHASE_AUTO_SWITCH 41
#define FID_GET_PHASE_AUTO_SWITCH 42
#define FID_SET_PHASES_CONNECTED 43
#define FID_GET_PHASES_CONNECTED 44
#define FID_SET_CHARGING_PROTOCOL 46
#define FID_GET_CHARGING_PROTOCOL 47
#define FID_SET_EICHRECHT_GENERAL_INFORMATION 48
#define FID_GET_EICHRECHT_GENERAL_INFORMATION 49
#define FID_SET_EICHRECHT_USER_ASSIGNMENT 50
#define FID_GET_EICHRECHT_USER_ASSIGNMENT 51
#define FID_SET_EICHRECHT_CHARGE_POINT 52
#define FID_GET_EICHRECHT_CHARGE_POINT 53
#define FID_SET_EICHRECHT_TRANSACTION 54
#define FID_GET_EICHRECHT_TRANSACTION 55
#define FID_GET_EICHRECHT_PUBLIC_KEY 56
#define FID_SET_ENUMERATE_CONFIGURATION 59
#define FID_GET_ENUMERATE_CONFIGURATION 60
#define FID_SET_ENUMERATE_VALUE 61
#define FID_GET_ENUMERATE_VALUE 62
#define FID_SET_CP_RECONNECT_TIME 63
#define FID_GET_CP_RECONNECT_TIME 64

#define FID_CALLBACK_ENERGY_METER_VALUES 45
#define FID_CALLBACK_EICHRECHT_DATASET_LOW_LEVEL 57
#define FID_CALLBACK_EICHRECHT_SIGNATURE_LOW_LEVEL 58

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
	bool car_stopped_charging;
	uint32_t time_since_state_change;
	uint32_t time_since_dc_fault_check;
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
	float current[3];
	uint8_t phases_active[1];
	uint8_t phases_connected[1];
} __attribute__((__packed__)) GetEnergyMeterValues_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllEnergyMeterValuesLowLevel;

typedef struct {
	TFPMessageHeader header;
	uint16_t values_length;
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
	uint16_t color_h;
	uint8_t color_s;
	uint8_t color_v;
} __attribute__((__packed__)) GetIndicatorLED_Response;

typedef struct {
	TFPMessageHeader header;
	int16_t indication;
	uint16_t duration;
	uint16_t color_h;
	uint8_t color_s;
	uint8_t color_v;
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
	bool ev_wakeup_enabled;
} __attribute__((__packed__)) SetEVWakeup;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEVWakuep;

typedef struct {
	TFPMessageHeader header;
	bool ev_wakeup_enabled;
} __attribute__((__packed__)) GetEVWakuep_Response;

typedef struct {
	TFPMessageHeader header;
	bool control_pilot_disconnect;
} __attribute__((__packed__)) SetControlPilotDisconnect;

typedef struct {
	TFPMessageHeader header;
	bool is_control_pilot_disconnect;
} __attribute__((__packed__)) SetControlPilotDisconnect_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetControlPilotDisconnect;

typedef struct {
	TFPMessageHeader header;
	bool control_pilot_disconnect;
} __attribute__((__packed__)) GetControlPilotDisconnect_Response;

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
	float current[3];
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
	uint16_t color_h;
	uint8_t color_s;
	uint8_t color_v;
	uint8_t button_configuration;
	uint32_t button_press_time;
	uint32_t button_release_time;
	bool button_pressed;
	bool ev_wakeup_enabled;
	bool control_pilot_disconnect;
	bool boost_mode_enabled;
	int16_t temperature;
	uint8_t phases_current;
	uint8_t phases_requested;
	uint8_t phases_state;
	uint8_t phases_info;
	bool phase_auto_switch_enabled;
	uint8_t phases_connected;
	uint8_t enumerate_value;
	uint32_t enumerate_value_change_time;
	uint8_t cp_reconnect_time;
} __attribute__((__packed__)) GetAllData2_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t password;
} __attribute__((__packed__)) FactoryReset;

typedef struct {
	TFPMessageHeader header;
	bool reset;
} __attribute__((__packed__)) GetButtonPressBootTime;

typedef struct {
	TFPMessageHeader header;
	uint32_t button_press_boot_time;
} __attribute__((__packed__)) GetButtonPressBootTime_Response;

typedef struct {
	TFPMessageHeader header;
	bool boost_mode_enabled;
} __attribute__((__packed__)) SetBoostMode;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetBoostMode;

typedef struct {
	TFPMessageHeader header;
	bool boost_mode_enabled;
} __attribute__((__packed__)) GetBoostMode_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t password;
} __attribute__((__packed__)) TriggerDCFaultTest;

typedef struct {
	TFPMessageHeader header;
	bool started;
} __attribute__((__packed__)) TriggerDCFaultTest_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t gp_output;
} __attribute__((__packed__)) SetGPOutput;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetTemperature;

typedef struct {
	TFPMessageHeader header;
	int16_t temperature;
} __attribute__((__packed__)) GetTemperature_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t phases;
} __attribute__((__packed__)) SetPhaseControl;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetPhaseControl;

typedef struct {
	TFPMessageHeader header;
	uint8_t phases_current;
	uint8_t phases_requested;
	uint8_t phases_state;
	uint8_t phases_info;
} __attribute__((__packed__)) GetPhaseControl_Response;

typedef struct {
	TFPMessageHeader header;
	bool phase_auto_switch_enabled;
} __attribute__((__packed__)) SetPhaseAutoSwitch;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetPhaseAutoSwitch;

typedef struct {
	TFPMessageHeader header;
	bool phase_auto_switch_enabled;
} __attribute__((__packed__)) GetPhaseAutoSwitch_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t phases_connected;
} __attribute__((__packed__)) SetPhasesConnected;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetPhasesConnected;

typedef struct {
	TFPMessageHeader header;
	uint8_t phases_connected;
} __attribute__((__packed__)) GetPhasesConnected_Response;

typedef struct {
	TFPMessageHeader header;
	float power;
	float current[3];
	uint8_t phases_active[1];
	uint8_t phases_connected[1];
} __attribute__((__packed__)) EnergyMeterValues_Callback;

typedef struct {
	TFPMessageHeader header;
	uint8_t charging_protocol;
	uint16_t cp_duty_cycle;
} __attribute__((__packed__)) SetChargingProtocol;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetChargingProtocol;

typedef struct {
	TFPMessageHeader header;
	uint8_t charging_protocol;
	uint16_t cp_duty_cycle;
} __attribute__((__packed__)) GetChargingProtocol_Response;

typedef struct {
	TFPMessageHeader header;
	char gateway_identification[32];
	char gateway_serial[32];
} __attribute__((__packed__)) SetEichrechtGeneralInformation;

typedef struct {
	TFPMessageHeader header;
	uint8_t eichrecht_state;
} __attribute__((__packed__)) SetEichrechtGeneralInformation_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEichrechtGeneralInformation;

typedef struct {
	TFPMessageHeader header;
	char gateway_identification[32];
	char gateway_serial[32];
} __attribute__((__packed__)) GetEichrechtGeneralInformation_Response;

typedef struct {
	TFPMessageHeader header;
	bool identification_status;
	uint8_t identification_flags[4];
	uint8_t identification_type;
	char identification_data[40];
} __attribute__((__packed__)) SetEichrechtUserAssignment;

typedef struct {
	TFPMessageHeader header;
	uint8_t eichrecht_state;
} __attribute__((__packed__)) SetEichrechtUserAssignment_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEichrechtUserAssignment;

typedef struct {
	TFPMessageHeader header;
	bool identification_status;
	uint8_t identification_flags[4];
	uint8_t identification_type;
	char identification_data[40];
} __attribute__((__packed__)) GetEichrechtUserAssignment_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t identification_type;
	char identification[20];
} __attribute__((__packed__)) SetEichrechtChargePoint;

typedef struct {
	TFPMessageHeader header;
	uint8_t eichrecht_state;
} __attribute__((__packed__)) SetEichrechtChargePoint_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEichrechtChargePoint;

typedef struct {
	TFPMessageHeader header;
	uint8_t identification_type;
	char identification[20];
} __attribute__((__packed__)) GetEichrechtChargePoint_Response;

typedef struct {
	TFPMessageHeader header;
	char transaction;
	uint32_t unix_time;
	int16_t utc_time_offset;
	uint16_t signature_format;
} __attribute__((__packed__)) SetEichrechtTransaction;

typedef struct {
	TFPMessageHeader header;
	uint8_t eichrecht_state;
} __attribute__((__packed__)) SetEichrechtTransaction_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEichrechtTransaction;

typedef struct {
	TFPMessageHeader header;
	char transaction;
	uint8_t transaction_state;
	uint8_t transaction_inner_state;
	uint16_t measurement_status;
	uint16_t signature_status;
	uint8_t eichrecht_state;
} __attribute__((__packed__)) GetEichrechtTransaction_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEichrechtPublicKey;

typedef struct {
	TFPMessageHeader header;
	uint8_t public_key[64];
} __attribute__((__packed__)) GetEichrechtPublicKey_Response;

typedef struct {
	TFPMessageHeader header;
	uint16_t message_length;
	uint16_t message_chunk_offset;
	char message_chunk_data[60];
} __attribute__((__packed__)) EichrechtDatasetLowLevel_Callback;

typedef struct {
	TFPMessageHeader header;
	uint16_t message_length;
	uint16_t message_chunk_offset;
	char message_chunk_data[60];
} __attribute__((__packed__)) EichrechtSignatureLowLevel_Callback;

typedef struct {
	TFPMessageHeader header;
	uint16_t enumerator_h[8];
	uint8_t enumerator_s[8];
	uint8_t enumerator_v[8];
} __attribute__((__packed__)) SetEnumerateConfiguration;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEnumerateConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint16_t enumerator_h[8];
	uint8_t enumerator_s[8];
	uint8_t enumerator_v[8];
} __attribute__((__packed__)) GetEnumerateConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t value;
} __attribute__((__packed__)) SetEnumerateValue;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetEnumerateValue;

typedef struct {
	TFPMessageHeader header;
	uint8_t value;
	uint32_t value_change_time;
} __attribute__((__packed__)) GetEnumerateValue_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t cp_reconnect_time;
} __attribute__((__packed__)) SetCPReconnectTime;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetCPReconnectTime;

typedef struct {
	TFPMessageHeader header;
	uint8_t cp_reconnect_time;
} __attribute__((__packed__)) GetCPReconnectTime_Response;


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
BootloaderHandleMessageResponse set_ev_wakeup(const SetEVWakeup *data);
BootloaderHandleMessageResponse get_ev_wakuep(const GetEVWakuep *data, GetEVWakuep_Response *response);
BootloaderHandleMessageResponse set_control_pilot_disconnect(const SetControlPilotDisconnect *data, SetControlPilotDisconnect_Response *response);
BootloaderHandleMessageResponse get_control_pilot_disconnect(const GetControlPilotDisconnect *data, GetControlPilotDisconnect_Response *response);
BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response);
BootloaderHandleMessageResponse get_all_data_2(const GetAllData2 *data, GetAllData2_Response *response);
BootloaderHandleMessageResponse factory_reset(const FactoryReset *data);
BootloaderHandleMessageResponse get_button_press_boot_time(const GetButtonPressBootTime *data, GetButtonPressBootTime_Response *response);
BootloaderHandleMessageResponse set_boost_mode(const SetBoostMode *data);
BootloaderHandleMessageResponse get_boost_mode(const GetBoostMode *data, GetBoostMode_Response *response);
BootloaderHandleMessageResponse trigger_dc_fault_test(const TriggerDCFaultTest *data, TriggerDCFaultTest_Response *response);
BootloaderHandleMessageResponse set_gp_output(const SetGPOutput *data);
BootloaderHandleMessageResponse get_temperature(const GetTemperature *data, GetTemperature_Response *response);
BootloaderHandleMessageResponse set_phase_control(const SetPhaseControl *data);
BootloaderHandleMessageResponse get_phase_control(const GetPhaseControl *data, GetPhaseControl_Response *response);
BootloaderHandleMessageResponse set_phase_auto_switch(const SetPhaseAutoSwitch *data);
BootloaderHandleMessageResponse get_phase_auto_switch(const GetPhaseAutoSwitch *data, GetPhaseAutoSwitch_Response *response);
BootloaderHandleMessageResponse set_phases_connected(const SetPhasesConnected *data);
BootloaderHandleMessageResponse get_phases_connected(const GetPhasesConnected *data, GetPhasesConnected_Response *response);
BootloaderHandleMessageResponse set_charging_protocol(const SetChargingProtocol *data);
BootloaderHandleMessageResponse get_charging_protocol(const GetChargingProtocol *data, GetChargingProtocol_Response *response);
BootloaderHandleMessageResponse set_eichrecht_general_information(const SetEichrechtGeneralInformation *data, SetEichrechtGeneralInformation_Response *response);
BootloaderHandleMessageResponse get_eichrecht_general_information(const GetEichrechtGeneralInformation *data, GetEichrechtGeneralInformation_Response *response);
BootloaderHandleMessageResponse set_eichrecht_user_assignment(const SetEichrechtUserAssignment *data, SetEichrechtUserAssignment_Response *response);
BootloaderHandleMessageResponse get_eichrecht_user_assignment(const GetEichrechtUserAssignment *data, GetEichrechtUserAssignment_Response *response);
BootloaderHandleMessageResponse set_eichrecht_charge_point(const SetEichrechtChargePoint *data, SetEichrechtChargePoint_Response *response);
BootloaderHandleMessageResponse get_eichrecht_charge_point(const GetEichrechtChargePoint *data, GetEichrechtChargePoint_Response *response);
BootloaderHandleMessageResponse set_eichrecht_transaction(const SetEichrechtTransaction *data, SetEichrechtTransaction_Response *response);
BootloaderHandleMessageResponse get_eichrecht_transaction(const GetEichrechtTransaction *data, GetEichrechtTransaction_Response *response);
BootloaderHandleMessageResponse get_eichrecht_public_key(const GetEichrechtPublicKey *data, GetEichrechtPublicKey_Response *response);
BootloaderHandleMessageResponse set_enumerate_configuration(const SetEnumerateConfiguration *data);
BootloaderHandleMessageResponse get_enumerate_configuration(const GetEnumerateConfiguration *data, GetEnumerateConfiguration_Response *response);
BootloaderHandleMessageResponse set_enumerate_value(const SetEnumerateValue *data);
BootloaderHandleMessageResponse get_enumerate_value(const GetEnumerateValue *data, GetEnumerateValue_Response *response);
BootloaderHandleMessageResponse set_cp_reconnect_time(const SetCPReconnectTime *data);
BootloaderHandleMessageResponse get_cp_reconnect_time(const GetCPReconnectTime *data, GetCPReconnectTime_Response *response);

// Callbacks
bool handle_energy_meter_values_callback(void);
bool handle_eichrecht_dataset_low_level_callback(void);
bool handle_eichrecht_signature_low_level_callback(void);

#define COMMUNICATION_CALLBACK_TICK_WAIT_MS 1
#define COMMUNICATION_CALLBACK_HANDLER_NUM 3
#define COMMUNICATION_CALLBACK_LIST_INIT \
	handle_energy_meter_values_callback, \
	handle_eichrecht_dataset_low_level_callback, \
	handle_eichrecht_signature_low_level_callback, \


#endif
