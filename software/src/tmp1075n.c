/* evse-v2-bricklet
 * Copyright (C) 2023 Olaf Lüke <olaf@tinkerforge.com>
 *
 * tmp1075n.c: Driver for TMP1075N temperature sensor
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

#include "tmp1075n.h"
#include "hardware_version.h"

TMP1075N tmp1075n;

void tmp1075n_init(void) {
    // No TMP1075N in EVSE V2
    if(hardware_version.is_v2) {
        return;
    } 

    memset(&tmp1075n, 0, sizeof(TMP1075N));
}

void tmp1075n_tick(void) {
    // No TMP1075N in EVSE V2
    if(hardware_version.is_v2) {
        return;
    } 
}
