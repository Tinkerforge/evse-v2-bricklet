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

#include "eichrecht.h"

#include "bricklib2/utility/util_definitions.h"

static const char *if_strings[] = {
    "RFID_NONE", "RFID_PLAIN", "RFID_RELATED", "RFID_PSK", "OCPP_NONE", "OCPP_RS", "OCPP_AUTH", "OCPP_RS_TLS", "OCPP_AUTH_TLS", "OCPP_CACHE", "OCPP_WHITELIST", "OCPP_CERTIFIED", "ISO15118_NONE", "ISO15118_PNC", "PLMN_NONE", "PLMN_RING", "PLMN_SMS"
};

static const char *it_strings[] = {
    "NONE", "DENIED", "UNDEFINED", "ISO14443", "ISO15693", "EMAID", "EVCCID", "EVCOID", "ISO7812", "CARD_TXN_NR", "CENTRAL", "CENTRAL_1", "CENTRAL_2", "LOCAL", "LOCAL_1", "LOCAL_2", "PHONE_NUMBER", "KEY_CODE"
};

static const char *ct_strings[] = {
    "EVSEID", "CBIDC"
};

Eichrecht eichrecht;

void eichrecht_init(void) {
    // Eichrecht support only on v4 hardware
    if(!hardware_version.is_v4) {
        return;
    }

    memset(&eichrecht, 0, sizeof(Eichrecht));
}

const char *eichrecht_get_if_string(uint8_t index) {
    if (index < ARRAY_SIZE(if_strings)) {
        return (char *)if_strings[index];
    } else {
        return NULL;
    }
}

const char *eichrecht_get_it_string(void) {
    if (eichrecht.ocmf.it < ARRAY_SIZE(it_strings)) {
        return (char *)it_strings[eichrecht.ocmf.it];
    } else {
        return "";
    }
}

const char *eichrecht_get_ct_string(void) {
    if (eichrecht.ocmf.ct < ARRAY_SIZE(ct_strings)) {
        return (char *)ct_strings[eichrecht.ocmf.ct];
    } else {
        return "";
    }
}

void eichrecht_create_dataset(void) {
    // WM3M4C manual:
    // JSON names must be in specified order and without whitespaces. Downloaded message should look like
    // {"FV":"1.0","GI":"","GS":"","PG":"","MV":"","MM":"","MS":"","MF":"","IS":true,"IF":[],"IT":"NONE","ID":"","CT":"EVSEID","CI":"","RD":[]}
    // Example of valid JSON dataset (newlines are added for better readability):
    // "FV":"1.0",
    // "GI":"Gateway 1",
    // "GS":"123456789",
    // "PG":"",
    // "MV":"",
    // "MM":"",
    // "MS":"",
    // "MF": "",
    // "IS":true,
    // "IF":[
    // "RFID_PLAIN",
    // "OCPP_RS_TLS"
    // ],
    // "IT":"ISO14443",
    // "ID":"1F2D3A4F5506C7",
    // "CT":"EVSEID",
    // "CI":"",
    // "RD":[]
    // }

    memset(eichrecht.dataset, 0, sizeof(eichrecht.dataset));

    // The absolute maximum size of the dataset here is known (since all possible strings are predefined).
    // The dataset size was chosen big enough to hold the maximum possible size.
    // Thus we can use unsafe string functions here.
    char *ptr = eichrecht.dataset;

    ptr = stpcpy(ptr, "{\"FV\":\"1.0\",\"GI\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.gi);
    ptr = stpcpy(ptr, "\",\"GS\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.gs);
    ptr = stpcpy(ptr, "\",\"PG\":\"\",\"MV\":\"\",\"MM\":\"\",\"MS\":\"\",\"MF\":\"\",\"IS\":");
    ptr = stpcpy(ptr, eichrecht.ocmf.is ? "true" : "false");
    ptr = stpcpy(ptr, ",\"IF\":[");

    bool first = true;
    for (int i = 0; i < 4; i++) {
        char *if_string = eichrecht_get_if_string(eichrecht.ocmf.if_[i]);
        if (if_string) {
            if (!first) {
                *ptr++ = ',';
            }
            first = false;
            *ptr++ = '"';
            ptr = stpcpy(ptr, if_string);
            *ptr++ = '"';
        }
    }

    ptr = stpcpy(ptr, "],\"IT\":\"");
    ptr = stpcpy(ptr, eichrecht_get_it_string());
    ptr = stpcpy(ptr, "\",\"ID\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.id);
    ptr = stpcpy(ptr, "\",\"CT\":\"");
    ptr = stpcpy(ptr, eichrecht_get_ct_string());
    ptr = stpcpy(ptr, "\",\"CI\":\"");
    ptr = stpcpy(ptr, eichrecht.ocmf.ci);
    ptr = stpcpy(ptr, "\",\"RD\":[]}");
}


void eichrecht_tick(void) {
    // Eichrecht support only on v4 hardware
    if(!hardware_version.is_v4) {
        return;
    }

    if(eichrecht.new_transaction) {
        eichrecht_create_dataset();
        eichrecht.new_transaction = false;
    }
}