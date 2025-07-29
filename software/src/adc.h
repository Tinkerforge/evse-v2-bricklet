/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * adc.h: Driver for ADC
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

#ifndef ADC_H
#define ADC_H

#include "xmc_vadc.h"
#include "xmc_gpio.h"

#include <stdint.h>
#include <stdbool.h>


#define ADC_NUM_WITH_PWM 2
#define ADC_NUM 5

#define ADC_CHANNEL_VCP1 0
#define ADC_CHANNEL_VCP2 1
#define ADC_CHANNEL_VPP  2
#define ADC_CHANNEL_V12P 3
#define ADC_CHANNEL_V12M 4

#define ADC_POSITIVE_MEASUREMENT 0
#define ADC_NEGATIVE_MEASUREMENT 1

typedef struct {
	// Pin
	XMC_GPIO_PORT_t *port;
	uint8_t pin;

	// ADC config
	uint8_t result_reg;
	uint8_t channel_num;
	int8_t channel_alias;
	uint8_t group_index;
	XMC_VADC_GROUP_t *group;

	// ADC result
	uint64_t result_sum[2];
	uint32_t result_count[2];
	int64_t result[2];
	int32_t result_mv[2];
	uint8_t result_index[2];

	uint8_t ignore_count;

	uint32_t timeout;

	char name[5];
} ADC;

typedef struct {
	uint32_t resistance_counter;
	uint32_t cp_pe_resistance;
	uint32_t pp_pe_resistance;
	int32_t cp_pe_pwm_low_mv[2];
	uint32_t cp_pe_pwm_low_count[2];
	uint64_t cp_pe_pwm_low_sum[2];
	int64_t cp_pe_pwm_low_result[2];
} ADCResult;

extern ADC *adc;
extern ADCResult adc_result;

void adc_init(void);
void adc_tick(void);
void adc_enable_all(const bool all);
void adc_ignore_results(const uint8_t count);

#endif