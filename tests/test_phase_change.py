#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST     = "localhost"
PORT     = 4223
UID_EVSE = "2CpXU5"

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse_v2 import BrickletEVSEV2
import time
import sys


if __name__ == "__main__":
    ipcon = IPConnection()
    evse = BrickletEVSEV2(UID_EVSE, ipcon)
    ipcon.connect(HOST, PORT)

    print(evse.set_phase_control(int(sys.argv[1])))
    while True:
        time.sleep(0.25)
        print(evse.get_phase_control())

    ipcon.disconnect()
