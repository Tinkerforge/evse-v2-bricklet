/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * adc.c: Driver for ADC
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

#include "adc.h"

#include "configs/config_evse.h"

#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/utility/util_definitions.h"

#define ADC_DIODE_DROP 650

ADC adc[ADC_NUM] = {
	{ // Channel ADC_CHANNEL_VCP1  (w/o resistor)
		.port          = XMC_GPIO_PORT2,
		.pin           = 1,
		.result_reg    = 4,
		.channel_num   = 0,
		.channel_alias = 6,
		.group_index   = 0,
		.group         = VADC_G0,
		.name          = "VCP1"
	},
	{ // Channel ADC_CHANNEL_VCP2  (w/ resistor)
		.port          = XMC_GPIO_PORT2,
		.pin           = 3,
		.result_reg    = 4,
		.channel_num   = 0,
		.channel_alias = 5,
		.group_index   = 1,
		.group         = VADC_G1,
		.name          = "VCP2"
	},
	{ // Channel ADC_CHANNEL_VPP
		.port          = XMC_GPIO_PORT2,
		.pin           = 2,
		.result_reg    = 5,
		.channel_num   = 1,
		.channel_alias = 7,
		.group_index   = 0,
		.group         = VADC_G0,
		.name          = "VPP"
	},
	{ // Channel ADC_CHANNEL_V12P
		.port          = XMC_GPIO_PORT2,
		.pin           = 4,
		.result_reg    = 5,
		.channel_num   = 1,
		.channel_alias = 6,
		.group_index   = 1,
		.group         = VADC_G1,
		.name          = "V12P"
	},
	{ // Channel ADC_CHANNEL_V12M
		.port          = XMC_GPIO_PORT2,
		.pin           = 5,
		.result_reg    = 6,
		.channel_num   = 7,
		.channel_alias = -1,
		.group_index   = 1,
		.group         = VADC_G1,
		.name          = "V12M"
	},
};

ADCResult adc_result;

#define adc_conversion_done_irq IRQ_Hdlr_15

void __attribute__((optimize("-O3"))) __attribute__ ((section (".ram_code"))) adc_conversion_done_irq(void) {
	XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	__NOP();
	XMC_GPIO_SetOutputLow(EVSE_OUTPUT_GP_PIN);
}

void adc_init_adc(void) {
	for(uint8_t i = 0; i < ADC_NUM; i ++) {
		const XMC_GPIO_CONFIG_t input_pin_config = {
			.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
			.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD,
		};
		XMC_GPIO_Init(adc[i].port, adc[i].pin, &input_pin_config);
	}

	// This structure contains the Global related Configuration.
	const XMC_VADC_GLOBAL_CONFIG_t adc_global_config = {
		.boundary0 = (uint32_t) 0, // Lower boundary value for Normal comparison mode
		.boundary1 = (uint32_t) 0, // Upper boundary value for Normal comparison mode

		.class0 = {
			.sample_time_std_conv     = 31,                      // The Sample time is (2*tadci)
			.conversion_mode_standard = XMC_VADC_CONVMODE_12BIT, // 12bit conversion Selected

		},
		.class1 = {
			.sample_time_std_conv     = 31,                      // The Sample time is (2*tadci)
			.conversion_mode_standard = XMC_VADC_CONVMODE_12BIT, // 12bit conversion Selected

		},

		.data_reduction_control         = 0, // Accumulate 1 result values
		.wait_for_read_mode             = 0, // GLOBRES Register will not be overwritten until the previous value is read
		.event_gen_enable               = 0, // Result Event from GLOBRES is disabled
		.disable_sleep_mode_control     = 0  // Sleep mode is enabled
	};


	// Global iclass0 configuration
	const XMC_VADC_GLOBAL_CLASS_t adc_global_iclass_config = {
		.conversion_mode_standard = XMC_VADC_CONVMODE_12BIT,
		.sample_time_std_conv	  = 31,
	};

	// Global Result Register configuration structure
	XMC_VADC_RESULT_CONFIG_t adc_global_result_config = {
		.data_reduction_control = 0,  // Accumulate 1 result values
		.post_processing_mode   = XMC_VADC_DMM_REDUCTION_MODE,
		.wait_for_read_mode  	= 0, // Enabled
		.part_of_fifo       	= 0, // No FIFO
		.event_gen_enable   	= 0  // Disable Result event
	};

	// LLD Background Scan Init Structure
	const XMC_VADC_BACKGROUND_CONFIG_t adc_background_config = {
		.conv_start_mode   = XMC_VADC_STARTMODE_CIR,       // Conversion start mode selected as cancel inject repeat
		.req_src_priority  = XMC_VADC_GROUP_RS_PRIORITY_1, // Priority of the Background request source in the VADC module
		.trigger_signal    = XMC_VADC_REQ_TR_A,            // If Trigger needed then this denotes the Trigger signal
		.trigger_edge      = XMC_VADC_TRIGGER_EDGE_RISING,   // If Trigger needed then this denotes Trigger edge selected
		.gate_signal       = XMC_VADC_REQ_GT_A,			   // If Gating needed then this denotes the Gating signal
		.timer_mode        = 0,							   // Timer Mode Disabled
		.external_trigger  = 1,                            // Trigger is Enabled
		.req_src_interrupt = 1,                            // Background Request source interrupt Enabled
		.enable_auto_scan  = 0,
		.load_mode         = XMC_VADC_SCAN_LOAD_OVERWRITE
	};

	const XMC_VADC_GROUP_CONFIG_t group_init_handle = {
		.emux_config = {
			.stce_usage                  = 0, 					           // Use STCE when the setting changes
			.emux_mode                   = XMC_VADC_GROUP_EMUXMODE_SWCTRL, // Mode for Emux conversion
			.emux_coding                 = XMC_VADC_GROUP_EMUXCODE_BINARY, // Channel progression - binary format
			.starting_external_channel   = 1,                              // Channel starts at 0 for EMUX
			.connected_channel           = 0                               // Channel connected to EMUX
		},
		.class0 = {
			.sample_time_std_conv        = 31,                             // The Sample time is (2*tadci)
			.conversion_mode_standard    = XMC_VADC_CONVMODE_12BIT,        // 12bit conversion Selected
			.sampling_phase_emux_channel = 0,                              // The Sample time is (2*tadci)
			.conversion_mode_emux        = XMC_VADC_CONVMODE_12BIT         // 12bit conversion Selected
		},
		.class1 = {
			.sample_time_std_conv        = 31,                             // The Sample time is (2*tadci)
			.conversion_mode_standard    = XMC_VADC_CONVMODE_12BIT,        // 12bit conversion Selected
			.sampling_phase_emux_channel = 0,                              // The Sample time is (2*tadci)
			.conversion_mode_emux        = XMC_VADC_CONVMODE_12BIT         // 12bit conversion Selected
		},

		.boundary0                       = 0,                              // Lower boundary value for Normal comparison mode
		.boundary1	                     = 0,                              // Upper boundary value for Normal comparison mode
		.arbitration_round_length        = 0,                              // 4 arbitration slots per round selected (tarb = 4*tadcd) */
		.arbiter_mode                    = XMC_VADC_GROUP_ARBMODE_ALWAYS,  // Determines when the arbiter should run.
	};

	XMC_VADC_GLOBAL_Init(VADC, &adc_global_config);

	XMC_VADC_GROUP_Init(VADC_G0, &group_init_handle);
	XMC_VADC_GROUP_Init(VADC_G1, &group_init_handle);
	XMC_VADC_GROUP_SetPowerMode(VADC_G0, XMC_VADC_GROUP_POWERMODE_NORMAL);
	XMC_VADC_GROUP_SetPowerMode(VADC_G1, XMC_VADC_GROUP_POWERMODE_NORMAL);

	XMC_VADC_GLOBAL_SHS_EnableAcceleratedMode(SHS0, XMC_VADC_GROUP_INDEX_0);
	XMC_VADC_GLOBAL_SHS_EnableAcceleratedMode(SHS0, XMC_VADC_GROUP_INDEX_1);
	XMC_VADC_GLOBAL_SHS_SetAnalogReference(SHS0, XMC_VADC_GLOBAL_SHS_AREF_EXTERNAL_VDD_UPPER_RANGE);

	XMC_VADC_GLOBAL_StartupCalibration(VADC);

	// Initialize the Global Conversion class 0
	XMC_VADC_GLOBAL_InputClassInit(VADC, adc_global_iclass_config, XMC_VADC_GROUP_CONV_STD, 0);

	// Initialize the Background Scan hardware
	XMC_VADC_GLOBAL_BackgroundInit(VADC, &adc_background_config);

	// Initialize the global result register
	XMC_VADC_GLOBAL_ResultInit(VADC, &adc_global_result_config);

	for(uint8_t i = 0; i < ADC_NUM; i ++) {
		XMC_VADC_CHANNEL_CONFIG_t  channel_config = {
			.input_class                =  XMC_VADC_CHANNEL_CONV_GLOBAL_CLASS0,    // Global ICLASS 0 selected
			.lower_boundary_select 	    =  XMC_VADC_CHANNEL_BOUNDARY_GROUP_BOUND0,
			.upper_boundary_select 	    =  XMC_VADC_CHANNEL_BOUNDARY_GROUP_BOUND0,
			.event_gen_criteria         =  XMC_VADC_CHANNEL_EVGEN_NEVER,           // Channel Event disabled
			.sync_conversion  		    =  0,                                      // Sync feature disabled
			.alternate_reference        =  XMC_VADC_CHANNEL_REF_INTREF,            // Internal reference selected
			.result_reg_number          =  adc[i].result_reg,                  // GxRES[10] selected
			.use_global_result          =  0, 				                       // Use Group result register
			.result_alignment           =  XMC_VADC_RESULT_ALIGN_RIGHT,            // Result alignment - Right Aligned
			.broken_wire_detect_channel =  XMC_VADC_CHANNEL_BWDCH_VAGND,           // No Broken wire mode select
			.broken_wire_detect		    =  0,    		                           // No Broken wire detection
			.bfl                        =  0,                                      // No Boundary flag
			.channel_priority           =  0,                   		           // Lowest Priority 0 selected
			.alias_channel              =  adc[i].channel_alias                // Channel is Aliased
		};

		XMC_VADC_RESULT_CONFIG_t channel_result_config = {
			.data_reduction_control = 0,                         // Accumulate 1 result values
			.post_processing_mode   = XMC_VADC_DMM_REDUCTION_MODE,  // Use reduction mode
			.wait_for_read_mode  	= 0,                            // Enabled
			.part_of_fifo       	= 0,                            // No FIFO
			.event_gen_enable   	= 0                             // Disable Result event
		};


		// Initialize for configured channels
		XMC_VADC_GROUP_ChannelInit(adc[i].group, adc[i].channel_num, &channel_config);

		// Initialize for configured result registers
		XMC_VADC_GROUP_ResultInit(adc[i].group, adc[i].result_reg, &channel_result_config);

		XMC_VADC_GLOBAL_BackgroundAddChannelToSequence(VADC, adc[i].group_index, adc[i].channel_num);
	}

	// Uncomment to turn on debug in IRQ (toggles GP output pin when adc conversion is ready)
//	XMC_VADC_GLOBAL_SetResultEventInterruptNode(VADC, XMC_VADC_SR_SHARED_SR0);

//	NVIC_SetPriority(15, 2);
//	XMC_VADC_GLOBAL_BackgroundSetReqSrcEventInterruptNode(VADC, XMC_VADC_SR_SHARED_SR0);
//	NVIC_EnableIRQ(15);
}

void adc_init(void) {
	adc_init_adc();
	adc->timeout = system_timer_get_ms();

	memset(&adc_result, 0, sizeof(ADCResult));
	adc_result.cp_pe_resistance = 0xFFFFFFFF;
	adc_result.pp_pe_resistance = 0xFFFFFFFF;
}

void adc_ignore_results(const uint8_t count) {
	for(uint8_t i = 0; i < ADC_NUM; i++) {
		adc[i].ignore_count = MAX(adc[i].ignore_count, count);
	}
}

void adc_enable_all(const bool all) {
	if(all) { // If PWM is off we evaluate all ADC channels
		for(uint8_t i = 0; i < ADC_NUM; i++) {
			XMC_VADC_GLOBAL_BackgroundAddChannelToSequence(VADC, adc[i].group_index, adc[i].channel_num);
		}
 	} else { // If PWM is on we only evaluate CP/PE
		for(uint8_t i = 0; i < ADC_NUM_WITH_PWM; i++) {
			XMC_VADC_GLOBAL_BackgroundAddChannelToSequence(VADC, adc[i].group_index, adc[i].channel_num);
		}
		for(uint8_t i = ADC_NUM_WITH_PWM; i < ADC_NUM; i++) {
			XMC_VADC_GLOBAL_BackgroundRemoveChannelFromSequence(VADC, adc[i].group_index, adc[i].channel_num);
		}
	}
}

void adc_check_result(const uint8_t i) {
	uint32_t result = XMC_VADC_GROUP_GetDetailedResult(adc[i].group, adc[i].result_reg);
	if(result & (1 << 31)) {
		uint16_t r = result & 0xFFFF;
		if((i <= 1) && (r < 2048)) {
			adc[i].result_sum[1] += r;
			adc[i].result_count[1]++;
		} else {
			adc[i].result_sum[0] += r;
			adc[i].result_count[0]++;
		}

		if(i == 0) {
			adc->timeout = system_timer_get_ms();
		}
	} else {
		if((i == 0) && system_timer_is_time_elapsed_ms(adc->timeout, 60000)) {
			// Trigger watchdog if we did not get a new result for adc channel 0 for 60 seconds.
			// In this case something went horribly wrong, since the adc should be triggered automatically in the background at all times.
			while(true) {
				__NOP();
			}
		}
	}
}

void adc_check_count(const uint8_t i) {
	if(i <= 1) {
		if(adc[i].result_count[1] >= 50) {
			adc[i].result_mv[1]    = (adc[i].result[1]*600*3300/4095-990*1000)/75;

			adc[i].result[1]       = adc[i].result_sum[1]/adc[i].result_count[1];
			adc[i].result_sum[1]   = 0;
			adc[i].result_count[1] = 0;
		}
	}

	if(adc[i].result_count[0] >= 50) {
		adc[i].result[0] = adc[i].result_sum[0]/adc[i].result_count[0];

		adc[i].result_sum[0] = 0;
		adc[i].result_count[0] = 0;

		// Return if ADC count counter > 0
		if(adc[i].ignore_count > 0) {
			adc[i].ignore_count--;
			return;
		}

		//uint32_t new_time = system_timer_get_ms();

		if(i == 2) { // PP/PE
			adc[i].result_mv[0] = adc[i].result[0]*3300/4095;

			// Rpp = (Vpp*1k*2k)/(5V*2k-Vpp*(1k+2k))
			const uint32_t divisor = 5000*2 - adc[2].result_mv[0]*(1+2);
			if(divisor == 0) {
				adc_result.pp_pe_resistance = 0xFFFFFFFF;
			} else {
				adc_result.pp_pe_resistance = 1000*2*adc[2].result_mv[0]/divisor;
				if(adc_result.pp_pe_resistance > 10000) {
					adc_result.pp_pe_resistance = 0xFFFFFFFF;
				}
			}
		} if(i == 3) { // +12V rail
			adc[i].result_mv[0] = adc[i].result[0]*4*3300/4095;
		} else {
			adc[i].result_mv[0] = (adc[i].result[0]*600*3300/4095-990*1000)/75;
			if(i == 1) {
				adc_result.resistance_counter++;

				// resistance divider, 910 ohm on EVSE
				// diode voltage drop 650mV (value is educated guess)
				if(adc[0].result_mv[0] <= adc[1].result_mv[0]) {
					adc_result.cp_pe_resistance = 0xFFFFFFFF;
				} else {
					const uint32_t divisor = adc[0].result_mv[0] - adc[1].result_mv[0];
					adc_result.cp_pe_resistance = 910*(adc[1].result_mv[0] - ADC_DIODE_DROP)/divisor;
					if(adc_result.cp_pe_resistance > 32000) {
						adc_result.cp_pe_resistance = 0xFFFFFFFF;
					}
				}
			}
		};
	}
}

void adc_tick(void) {
	for(uint8_t i = 0; i < ADC_NUM; i++) {
		adc_check_result(i);
	}

	for(uint8_t i = 0; i < ADC_NUM; i++) {
		adc_check_count(i);
	}
}
