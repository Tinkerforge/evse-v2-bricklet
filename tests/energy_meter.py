#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST     = "localhost"
PORT     = 4223
UID_EVSE = "2sTvA3"

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse_v2 import BrickletEVSEV2
import time
import sys


if __name__ == "__main__":
    ipcon = IPConnection()
    evse = BrickletEVSEV2(UID_EVSE, ipcon)
    ipcon.connect(HOST, PORT)

    values = evse.get_energy_meter_detailed_values()
    print("""
line_to_neutral_volts[SDM630_PHASE_NUM]  = {}, {}, {}
current[SDM630_PHASE_NUM]                = {}, {}, {}
power[SDM630_PHASE_NUM]                  = {}, {}, {}
volt_amps[SDM630_PHASE_NUM]              = {}, {}, {}
volt_amps_reactive[SDM630_PHASE_NUM]     = {}, {}, {}
power_factor[SDM630_PHASE_NUM]           = {}, {}, {}
phase_angle[SDM630_PHASE_NUM]            = {}, {}, {}
average_line_to_neutral_volts            = {}
average_line_current                     = {}
sum_of_line_currents                     = {}
total_system_power                       = {}
total_system_volt_amps                   = {}
total_system_var                         = {}
total_system_power_factor                = {}
total_system_phase_angle                 = {}
frequency_of_supply_voltages             = {}
total_import_kwh                         = {}
total_export_kwh                         = {}
total_import_kvarh                       = {}
total_export_kvarh                       = {}
total_vah                                = {}
ah                                       = {}
total_system_power_demand                = {}
maximum_total_system_power_demand        = {}
total_system_va_demand                   = {}
maximum_total_system_va_demand           = {}
neutral_current_demand                   = {}
maximum_neutral_current_demand           = {}
line1_to_line2_volts                     = {}
line2_to_line3_volts                     = {}
line3_to_line1_volts                     = {}
average_line_to_line_volts               = {}
neutral_current                          = {}
ln_volts_thd[SDM630_PHASE_NUM]           = {}, {}, {}
current_thd[SDM630_PHASE_NUM]            = {}, {}, {}
average_line_to_neutral_volts_thd        = {}
current_demand[SDM630_PHASE_NUM]         = {}, {}, {}
maximum_current_demand[SDM630_PHASE_NUM] = {}, {}, {}
line1_to_line2_volts_thd                 = {}
line2_to_line3_volts_thd                 = {}
line3_to_line1_volts_thd                 = {}
average_line_to_line_volts_thd           = {}
total_kwh_sum                            = {}
total_kvarh_sum                          = {}
import_kwh[SDM630_PHASE_NUM]             = {}, {}, {}
export_kwh[SDM630_PHASE_NUM]             = {}, {}, {}
total_kwh[SDM630_PHASE_NUM]              = {}, {}, {}
import_kvarh[SDM630_PHASE_NUM]           = {}, {}, {}
export_kvarh[SDM630_PHASE_NUM]           = {}, {}, {}
total_kvarh[SDM630_PHASE_NUM]            = {}, {}, {}
""".format(*values)
)
    ipcon.disconnect()
