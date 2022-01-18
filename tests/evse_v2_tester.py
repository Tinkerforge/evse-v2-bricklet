#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST     = "localhost"
PORT     = 4223

UID_EVSE = None
UID_IDAI = "Sii"
#UID_IDI4 = "T8j"
UID_IQR1 = "UmP" # CP
UID_IQR2 = "Ukk" # PP
UID_IQR3 = "Ukd" # ?
UID_ICOU = "Gh4"
UID_IDR  = "Tfh" # L, L'

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse_v2 import BrickletEVSEV2

from tinkerforge.bricklet_industrial_dual_relay        import BrickletIndustrialDualRelay
from tinkerforge.bricklet_industrial_dual_analog_in_v2 import BrickletIndustrialDualAnalogInV2
from tinkerforge.bricklet_industrial_digital_out_4_v2  import BrickletIndustrialDigitalOut4V2
from tinkerforge.bricklet_industrial_digital_in_4_v2   import BrickletIndustrialDigitalIn4V2
from tinkerforge.bricklet_industrial_quad_relay_v2     import BrickletIndustrialQuadRelayV2
from tinkerforge.bricklet_industrial_counter           import BrickletIndustrialCounter

import time
import sys

log = print 

class EVSEV2Tester:
    def __init__(self, log_func = None):
        if log_func:
            global log
            log = log_func

        self.ipcon = IPConnection()
        self.ipcon.connect(HOST, PORT)
        self.ipcon.register_callback(IPConnection.CALLBACK_ENUMERATE, self.cb_enumerate)
        self.ipcon.enumerate()

        log("Trying to find EVSE Bricklet...")
        while UID_EVSE == None:
            time.sleep(0.1)
        log("Found EVSE Bricklet: {0}".format(UID_EVSE))

        self.evse = BrickletEVSEV2(UID_EVSE, self.ipcon)
    
        self.idai = BrickletIndustrialDualAnalogInV2(UID_IDAI, self.ipcon)
        self.idr = BrickletIndustrialDualRelay(UID_IDR,  self.ipcon) 
#        self.idi4 = BrickletIndustrialDigitalIn4V2(UID_IDI4,   self.ipcon)
        self.iqr1 = BrickletIndustrialQuadRelayV2(UID_IQR1,    self.ipcon)
        self.iqr2 = BrickletIndustrialQuadRelayV2(UID_IQR2,    self.ipcon)
        self.iqr3 = BrickletIndustrialQuadRelayV2(UID_IQR3,    self.ipcon)
#        self.icou = BrickletIndustrialCounter(UID_ICOU,        self.ipcon)
        self.idai.set_sample_rate(self.idai.SAMPLE_RATE_61_SPS)

    def cb_enumerate(self, uid, connected_uid, position, hardware_version, firmware_version, device_identifier, enumeration_type):
        global UID_EVSE
        if device_identifier == BrickletEVSEV2.DEVICE_IDENTIFIER:
            UID_EVSE = uid

    # Live = True
    def set_contactor(self, contactor_input, contactor_output):
        if contactor_input:
            self.idr.set_selected_value(0, True)
            log('AC0 live')
        else:
            self.idr.set_selected_value(0, False)
            log('AC0 off')

        if contactor_output:
            self.idr.set_selected_value(1, True)
            log('AC1 live')
        else:
            self.idr.set_selected_value(1, False)
            log('AC1 off')

    def set_diode(self, enable):
        value = list(self.iqr1.get_value())
        value[0] = enable
        self.iqr1.set_value(value)
        if enable:
            log("Enable lock switch configuration diode")
        else:
            log("Disable lock switch configuration diode")

    def get_cp_pe_voltage(self):
        return self.idai.get_voltage(1)

    def set_cp_pe_resistor(self, r2700, r880, r240):
        value = list(self.iqr1.get_value())
        value[1] = r2700
        value[2] = r880
        value[3] = r240
        self.iqr1.set_value(value)

        l = []
        if r2700: l.append("2700 Ohm")
        if r880:  l.append("880 Ohm")
        if r240:  l.append("240 Ohm")
    
        log("Set CP/PE resistor: " + ', '.join(l))

    def set_pp_pe_resistor(self, r1500, r680, r220, r100):
        value = [r1500, r680, r220, r100]
        self.iqr2.set_value(value)

        l = []
        if r1500: l.append("1500 Ohm")
        if r680:  l.append("680 Ohm")
        if r220:  l.append("220 Ohm")
        if r100:  l.append("110 Ohm")
    
        log("Set PP/PE resistor: " + ', '.join(l)) 

    def wait_for_contactor_gpio(self, active):
        if active:
            log("Waiting for contactor GPIO to become active...")
        else:
            log("Waiting for contactor GPIO to become inactive...")

        while True:
            state = self.evse.get_low_level_state()
            if state.gpio[9] == active:
                break
            time.sleep(0.01)

        log("Done")

    def wait_for_button_gpio(self, active):
        if active:
            log("Waiting for button GPIO to become active...")
        else:
            log("Waiting for button GPIO to become inactive...")

        while True:
            state = self.evse.get_low_level_state()
            if state.gpio[6] == active:
                break
            time.sleep(0.1)

        log("Done")

    def set_max_charging_current(self, current):
        self.evse.set_charging_slot(5, current, True, False)

    def shutdown_input_enable(self, enable):
        self.iqr3.set_selected_value(0, enable)
    
    def get_energy_meter_data(self):
        a = self.evse.get_energy_meter_values()
        b = self.evse.get_all_energy_meter_values()
        c = self.evse.get_hardware_configuration()
        d = self.evse.get_energy_meter_errors()
        return (a, b, c, d)
    
    def get_cp_pe_voltage(self):
        return self.idai.get_voltage(1)

    def get_pp_pe_voltage(self):
        return self.idai.get_voltage(0)

    def exit(self, value):
        self.iqr1.set_value([False]*4)
        self.iqr2.set_value([False]*4)
        self.iqr3.set_value([False]*4)
        self.idr.set_value(False, False)
        sys.exit(value)

if __name__ == "__main__":
    evse_tester = EVSEV2Tester()

    # Initial config
    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()
    time.sleep(5)

    while True:
        time.sleep(5)
        evse_tester.set_cp_pe_resistor(True, False, False)
        time.sleep(5)
        evse_tester.set_cp_pe_resistor(True, True, False)

        evse_tester.wait_for_contactor_gpio(True)
        evse_tester.set_contactor(True, True)

        time.sleep(10)
        evse_tester.set_cp_pe_resistor(True, False, False)
        evse_tester.wait_for_contactor_gpio(False)
        evse_tester.set_contactor(True, False)

        time.sleep(5)
        evse_tester.set_cp_pe_resistor(False, False, False)
        time.sleep(5)
