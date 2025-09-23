/* evse-v2-bricklet
 * Copyright (C) 2025 Olaf Lüke <olaf@tinkerforge.com>
 *
 * eichrecht.c: Eichrecht implementation
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

#ifndef EICHRECHT_H
#define EICHRECHT_H

#include "hardware_version.h"

typedef struct {
    // General Information
    char gi[33]; // Gateway Identification
    char gs[33]; // Gateway Serial

    // User Assignment
    bool is; // Identification Status
    uint8_t if_[4]; // Identification Flags
    uint8_t it; // Identification Type
    char id[41]; // Identification Data

    // Charge Point
    uint8_t ct; // Identification Type
    char ci[21]; // Identification

    // Transaction
    char tx; // Transaction
} OCMF;

typedef struct {
    OCMF ocmf;

    uint64_t unix_time;
    bool new_transaction;

    char dataset[1024];
} Eichrecht;

extern Eichrecht eichrecht;

void eichrecht_init(void);
void eichrecht_tick(void);

#endif