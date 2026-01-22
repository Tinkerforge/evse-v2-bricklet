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

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // General Information
    char gi[42]; // Gateway Identification
    char gs[26]; // Gateway Serial

    // User Assignment
    bool is; // Identification Status
    uint8_t if_[4]; // Identification Flags
    uint8_t it; // Identification Type
    char id[41]; // Identification Data

    // Charge Point
    uint8_t ct; // Identification Type
    char ci[21]; // Identification
} OCMF;

typedef struct {
    OCMF ocmf;

    bool init_done;

    uint8_t transaction_state;
    uint8_t transaction_inner_state;
    uint32_t transaction_state_time;

    char transaction;
    uint32_t unix_time;
    int16_t utc_time_offset;
    uint16_t signature_format;
    bool new_transaction;
    uint32_t timeout_counter;

    char dataset_in[512] __attribute__((aligned(4)));
    uint16_t dataset_in_index;

    char dataset_out[1024] __attribute__((aligned(4)));
    uint16_t dataset_out_index;
    uint16_t dataset_out_length;
    bool dataset_out_ready;
    uint16_t dataset_out_chunk_offset;

    char signature[256] __attribute__((aligned(4)));
    uint16_t signature_index;
    uint16_t signature_length;
    bool signature_ready;
    uint16_t signature_chunk_offset;

    char public_key[64] __attribute__((aligned(4)));
} Eichrecht;

extern Eichrecht eichrecht;

void eichrecht_init(void);
void eichrecht_tick(void);
void eichrecht_iskra_tick(void);
void eichrecht_iskra_init_tick(void);

#endif
