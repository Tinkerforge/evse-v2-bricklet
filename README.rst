EVSE Bricklet 2.0
=================

This Bricklet together with the ESP32 Ethernet Brick and the NFC Bricklet
form the hardware used in the WARP2 Charger.

The Firmware for the EVSE Bricklet 2.0 is also used for the EVSE Bricklet 3.0:
https://github.com/Tinkerforge/evse-v3-bricklet. The hardware version is
run-time detected.

See https://www.warp-charger.com/ for more details about WARP Charger.

This repository contains the firmware source code and the hardware design
files. The documentation generator configs can be found at
https://github.com/Tinkerforge/generators

Repository Overview
-------------------

.. DO NOT EDIT THIS OVERVIEW MANUALLY! CHANGE https://github.com/Tinkerforge/esp32-firmware/repo_overview.rst AND COPY THAT BLOCK INTO ALL REPOS LISTED BELOW. TODO: AUTOMATE THIS

Software
~~~~~~~~
- `esp32-firmware <https://github.com/Tinkerforge/esp32-firmware>`__  **Please report any issues concerning WARP hard- and software here!** Source code of the ESP32 firmware shared between all WARP Chargers and Energy Managers

- `tfjson <https://github.com/Tinkerforge/tfjson>`__ SAX style JSON serializer and deserializer
- `tfmodbustcp <https://github.com/Tinkerforge/tfmodbustcp>`__ Modbus TCP server and client implementation
- `tfocpp <https://github.com/Tinkerforge/tfocpp>`__ OCPP 1.6 implementation
- `tftools <https://github.com/Tinkerforge/tftools>`__ Miscellaneous tools and helpers

- `esp32-remote-access <https://github.com/Tinkerforge/esp32-remote-access>`__ Source code of the my.warp-charger.com remote access server

- `warp-charger <https://github.com/Tinkerforge/warp-charger>`__ The source code of (docs.)warp-charger.com and the printed manual, released firmwares, datasheets and documents, as well as some tools and hardware design files
- `api.warp-charger.com <https://github.com/Tinkerforge/api.warp-charger.com>`__ Serves APIs that are used by WARP Chargers to obtain relevant public information like day ahead prices
- `vislog.warp-charger.com <https://github.com/Tinkerforge/vislog.warp-charger.com>`__ Visualizes WARP Charger logs and EVSE debug protocols
- `dbus-warp-charger <https://github.com/Tinkerforge/dbus-warp-charger>`__ Integrates WARP Chargers into a Victron Energy Venus OS device (e.g. Cerbo GX)

WARP Charger Hardware
~~~~~~~~~~~~~~~~~~~~~~

- `esp32-brick <https://github.com/Tinkerforge/esp32-brick>`__ Hardware design files of the ESP32 Brick
- `evse-bricklet <https://github.com/Tinkerforge/evse-bricklet>`__  Firmware source code and hardware design files of the EVSE Bricklet
- `rs485-bricklet <https://github.com/Tinkerforge/rs485-bricklet>`__ Firmware source code and hardware design files of the RS485 Bricklet


WARP2 Charger Hardware
~~~~~~~~~~~~~~~~~~~~~~

- `esp32-ethernet-brick <https://github.com/Tinkerforge/esp32-ethernet-brick>`__ Hardware design files of the ESP32 Ethernet Brick
- `evse-v2-bricklet <https://github.com/Tinkerforge/evse-v2-bricklet>`__ Firmware source code and hardware design files of the EVSE 2.0 Bricklet
- `nfc-bricklet <https://github.com/Tinkerforge/nfc-bricklet>`__ Firmware source code and hardware design files of the NFC Bricklet

WARP3 Charger Hardware
~~~~~~~~~~~~~~~~~~~~~~

- `warp-esp32-ethernet-brick <https://github.com/Tinkerforge/warp-esp32-ethernet-brick>`__ Hardware design files of the WARP ESP32 Ethernet Brick
- `evse-v3-bricklet <https://github.com/Tinkerforge/evse-v3-bricklet>`__ Firmware source code and hardware design files of the EVSE 3.0 Bricklet
- `nfc-bricklet <https://github.com/Tinkerforge/nfc-bricklet>`__ Firmware source code and hardware design files of the NFC Bricklet

WARP Energy Manager Hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- `esp32-ethernet-brick <https://github.com/Tinkerforge/esp32-ethernet-brick>`__ Hardware design files of the ESP32 Ethernet Brick
- `warp-energy-manager-bricklet <https://github.com/Tinkerforge/warp-energy-manager-bricklet>`__ Firmware source code and hardware design files of the WARP Energy Manager Bricklet

WARP Energy Manager 2.0 Hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- `esp32-ethernet-brick <https://github.com/Tinkerforge/esp32-ethernet-brick>`__ Hardware design files of the ESP32 Ethernet Brick
- `warp-energy-manager-v2-bricklet <https://github.com/Tinkerforge/warp-energy-manager-v2-bricklet>`__ Firmware source code and hardware design files of the WARP Energy Manager 2.0 Bricklet
- `warp-front-panel-bricklet <https://github.com/Tinkerforge/warp-front-panel-bricklet>`__ Firmware source code and hardware design files of the WARP Front Panel Bricklet

Forked/patched projects
~~~~~~~~~~~~~~~~~~~~~~~

- `arduino-esp32 <https://github.com/Tinkerforge/arduino-esp32>`__
- `esp32-arduino-libs <https://github.com/Tinkerforge/esp32-arduino-libs>`__
- `WireGuard-ESP32-Arduino <https://github.com/Tinkerforge/WireGuard-ESP32-Arduino>`__

Repository Content
------------------

software/:
 * examples/: Examples for all supported languages
 * build/: Compiled files
 * src/: Source code of firmware
 * Makefile: Makefile to build project

hardware/:
 * Contains KiCad project files and additionally schematics as PDF

datasheets/:
 * Contains datasheets for sensors and complex ICs that are used

Hardware
--------

The hardware is designed with the open source EDA Suite KiCad
(http://www.kicad.org). Before you are able to open the files,
you have to install the Tinkerforge kicad-libraries
(https://github.com/Tinkerforge/kicad-libraries). You can either clone
them directly in hardware/ or clone them in a separate folder and
symlink them into hardware/
(ln -s kicad_path/kicad-libraries project_path/hardware). After that you
can open the .pro file in hardware/ with KiCad and from there view and
modify the schematics and the PCB layout.

Software
--------

If you want to do your own Brick/Bricklet firmware development we highly
recommend that you use our build environment setup script and read the
tutorial (https://www.tinkerforge.com/en/doc/Tutorials/Tutorial_Build_Environment/Tutorial.html).

To compile the C code we recommend you to install the newest GNU Arm Embedded
Toolchain (https://launchpad.net/gcc-arm-embedded/+download).
You also need to install bricklib2 (https://github.com/Tinkerforge/bricklib2).

You can either clone it directly in software/src/ or clone it in a
separate folder and symlink it into software/src/
(ln -s bricklib_path/bricklib2 project_path/software/src/). Finally make sure to
have CMake installed (http://www.cmake.org/cmake/resources/software.html).

After that you can build the firmware by invoking make in software/.
The firmware (.zbin) can then be found in software/build/ and uploaded
with brickv (click button "Flashing" on start screen).
