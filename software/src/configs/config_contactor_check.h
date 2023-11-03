/* evse-v2-bricklet
 * Copyright (C) 2021-2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_contactor_check.h: Config for welded/defective contactor check
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

#ifndef CONTACTOR_CHECK_CONFIG_H
#define CONTACTOR_CHECK_CONFIG_H

#include "xmc_gpio.h"

// EVSE V2 (50Hz signal before and after contactor)
#define CONTACTOR_CHECK_AC1_PIN P2_6
#define CONTACTOR_CHECK_AC2_PIN P2_7
#define CONTACTOR_CHECK_RELAY_PIN_IS_INVERTED true

// EVSE V3 (High or low signal from contactor feedback)
#define CONTACTOR_CHECK_FB1_PIN P2_7 // Feedback contactor 1
#define CONTACTOR_CHECK_FB2_PIN P2_6 // Feedback contactor 2
// 50 Hz signal if PE is connected correctly
#define CONTACTOR_CHECK_PE_PIN  P2_8 // PE check

#endif
