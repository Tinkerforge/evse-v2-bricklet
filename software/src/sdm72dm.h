/* evse-v2-bricklet
 * Copyright (C) 2021 Olaf Lüke <olaf@tinkerforge.com>
 *
 * sdm72dm.h: SDM72DM driver
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

#ifndef SDM72DM_H
#define SDM72DM_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t state;

    union {
        float f;
        uint32_t data;
    } power;

    union {
        float f;
        uint32_t data;
    } energy_relative;

    union {
        float f;
        uint32_t data;
    } energy_absolute;

    bool available;

    bool reset_energy_meter;
} SDM72DM;

extern SDM72DM sdm72dm;

void sdm72dm_init(void);
void sdm72dm_tick(void);

#endif