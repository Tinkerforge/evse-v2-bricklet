/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * contactor_check.h: Welded/defective contactor check functions
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

#ifndef CONTACTOR_CHECK_H
#define CONTACTOR_CHECK_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_NLIVE = 0,
    CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_NLIVE  = 1,
    CONTACTOR_CHECK_STATE_AC1_NLIVE_AC2_LIVE  = 2,
    CONTACTOR_CHECK_STATE_AC1_LIVE_AC2_LIVE   = 3,
} ContactorCheckState;

typedef struct {
    uint32_t ac1_edge_count;
    uint32_t ac2_edge_count;

    bool ac1_last_value;
    bool ac2_last_value;

    uint32_t last_check;

    uint8_t invalid_counter;

    ContactorCheckState state;
    uint8_t error;
} ContactorCheck;

extern ContactorCheck contactor_check;

void contactor_check_init(void);
void contactor_check_tick(void);

#endif