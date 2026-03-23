/* evse-v2-bricklet
 * Copyright (C) 2026 Olaf Lüke <olaf@tinkerforge.com>
 *
 * frequency.c: Mains frequency measurement from PE check 50 Hz signal
 *
 * This runs in parallel to the existing PE presence detection in
 * contactor_check.c which only counts edges without measuring timing.
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

#include "frequency.h"

#include "configs/config_frequency.h"
#include "hardware_version.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/utility/util_definitions.h"

#include "xmc_ccu4.h"
#include "xmc_eru.h"
#include "xmc_scu.h"

#include <string.h>

Frequency frequency;

// ERU0_SR3 -> IRQ6: Called on each rising edge of the 50 Hz PE check signal
void __attribute__((optimize("-O3"))) IRQ_Hdlr_6(void) {
	const uint16_t current = (uint16_t)XMC_CCU4_SLICE_GetTimerValue(FREQUENCY_TIMER_SLICE);

	// Unsigned 16-bit subtraction handles timer wrap-around correctly
	const uint16_t delta = current - frequency.last_timer_value;

	// Only accept periods in the plausible range
	if(delta >= FREQUENCY_MIN_TICKS && delta <= FREQUENCY_MAX_TICKS) {
		frequency.period_buffer[frequency.buffer_index] = delta;
		frequency.buffer_index++;
	}

	frequency.last_timer_value = current;
	frequency.last_edge_ms = system_timer_get_ms();
}

void frequency_init(void) {
	memset(&frequency, 0, sizeof(Frequency));

	// Frequency measurement only available on V3
	if(!hardware_version.is_v3) {
		return;
	}

	XMC_CCU4_Init(FREQUENCY_TIMER_MODULE, XMC_CCU4_SLICE_MCMS_ACTION_TRANSFER_PR_CR);
	XMC_CCU4_StartPrescaler(FREQUENCY_TIMER_MODULE);

	const XMC_CCU4_SLICE_COMPARE_CONFIG_t timer_config = {
		.timer_mode          = XMC_CCU4_SLICE_TIMER_COUNT_MODE_EA,
		.monoshot            = false,
		.shadow_xfer_clear   = 0,
		.dither_timer_period = 0,
		.dither_duty_cycle   = 0,
		.prescaler_mode      = XMC_CCU4_SLICE_PRESCALER_MODE_NORMAL,
		.mcm_enable          = 0,
		.prescaler_initval   = FREQUENCY_TIMER_PRESCALER,
		.float_limit         = 0,
		.dither_limit        = 0,
		.passive_level       = XMC_CCU4_SLICE_OUTPUT_PASSIVE_LEVEL_LOW,
		.timer_concatenation = 0
	};

	XMC_CCU4_SLICE_CompareInit(FREQUENCY_TIMER_SLICE, &timer_config);
	XMC_CCU4_SLICE_SetTimerPeriodMatch(FREQUENCY_TIMER_SLICE, 0xFFFFU);
	XMC_CCU4_EnableShadowTransfer(FREQUENCY_TIMER_MODULE, XMC_CCU4_SHADOW_TRANSFER_SLICE_3 | XMC_CCU4_SHADOW_TRANSFER_PRESCALER_SLICE_3);
	XMC_CCU4_EnableClock(FREQUENCY_TIMER_MODULE, (uint8_t)FREQUENCY_TIMER_SLICE_NUM);
	XMC_CCU4_SLICE_StartTimer(FREQUENCY_TIMER_SLICE);

	// ETL3 Input B1 = P2_8, source = B only, detect rising edge
	// Output trigger to OGU3, which generates service request
	XMC_ERU_ETL_SetInput(FREQUENCY_ERU, FREQUENCY_ERU_ETL_CHANNEL, XMC_ERU_ETL_INPUT_A0, FREQUENCY_ERU_INPUT_B);
	XMC_ERU_ETL_SetSource(FREQUENCY_ERU, FREQUENCY_ERU_ETL_CHANNEL, XMC_ERU_ETL_SOURCE_B);
	XMC_ERU_ETL_SetEdgeDetection(FREQUENCY_ERU, FREQUENCY_ERU_ETL_CHANNEL, XMC_ERU_ETL_EDGE_DETECTION_RISING);
	XMC_ERU_ETL_EnableOutputTrigger(FREQUENCY_ERU, FREQUENCY_ERU_ETL_CHANNEL, XMC_ERU_ETL_OUTPUT_TRIGGER_CHANNEL3);

	XMC_ERU_OGU_SetServiceRequestMode(FREQUENCY_ERU, FREQUENCY_ERU_OGU_CHANNEL, XMC_ERU_OGU_SERVICE_REQUEST_ON_TRIGGER);

	// Route ERU0_SR3 to NVIC IRQ6 and enable
	NVIC_SetPriority((IRQn_Type)FREQUENCY_IRQ_NUM, 3U);
	XMC_SCU_SetInterruptControl(FREQUENCY_IRQ_NUM, FREQUENCY_IRQCTRL);
	NVIC_EnableIRQ((IRQn_Type)FREQUENCY_IRQ_NUM);

	frequency.start_ms = system_timer_get_ms();
	frequency.start_ms = MIN(1, frequency.start_ms);
}

void frequency_tick(void) {
	if(!hardware_version.is_v3) {
		return;
	}

	// Wait for 6s after start to be sure that full buffer is read (have to wait for at least 255/50 = 5.1s).
	if((frequency.start_ms != 0) && system_timer_is_time_elapsed_ms(frequency.start_ms, 6*1000)) {
		frequency.start_ms = 0;
		return;
	}

	// Check for timeout (PE disconnected or no 50 Hz present)
	if(frequency.last_edge_ms != 0 &&
	   system_timer_is_time_elapsed_ms(frequency.last_edge_ms, FREQUENCY_TIMEOUT_MS)) {
		frequency.valid = false;
		return;
	}

	// Update once per 0.5s
	if(system_timer_is_time_elapsed_ms(frequency.last_update, 500)) {
		uint32_t sum = 0;
		for(uint16_t i = 0; i < FREQUENCY_BUFFER_SIZE; i++) {
			sum += frequency.period_buffer[i];
		}

		frequency.frequency = ((((uint64_t)(FREQUENCY_MILLIHZ_FACTOR))*((uint64_t)FREQUENCY_BUFFER_SIZE)) / ((uint64_t)sum));
		frequency.valid = true;

		frequency.last_update = system_timer_get_ms();
	}
}
