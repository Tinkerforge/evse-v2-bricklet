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

.. BEGIN WARP REPOSITORIES (managed block, generated from esp32-firmware/repo_overview.rst - do not edit by hand, run update_repo_overview.py instead)

WARP Repositories
-----------------

This is an overview of all repositories that make up the WARP Charger and
WARP Energy Manager ecosystem.

Please report **any** issue concerning WARP hard- or software in the
`esp32-firmware`_ repository, no matter which repository it originates from.

Software
~~~~~~~~

- `esp32-firmware`_ - Source code of the ESP32 firmware shared between all WARP Chargers and Energy Managers

Libraries used by the firmware:

- `tfjson`_ - SAX style JSON serializer and deserializer
- `tfmodbustcp`_ - Modbus TCP server and client implementation
- `tfocpp`_ - OCPP 1.6 implementation
- `tftools`_ - Miscellaneous tools and helpers

Remote access:

- `esp32-remote-access`_ - Source code of the my.warp-charger.com remote access server

Documentation, tools and integrations:

- `warp-charger`_ - Source code of (docs.)warp-charger.com and the printed manual, released firmwares, datasheets and documents, as well as some tools and hardware design files
- `api.warp-charger.com`_ - Serves APIs that are used by WARP Chargers to obtain relevant public information like day ahead prices
- `vislog.warp-charger.com`_ - Visualizes WARP Charger logs and EVSE debug protocols
- `dbus-warp-charger`_ - Integrates WARP Chargers into a Victron Energy Venus OS device (e.g. Cerbo GX)

WARP Charger Hardware
~~~~~~~~~~~~~~~~~~~~~~

WARP4 Charger
"""""""""""""

*Current charger generation.*

- `warp-esp32-ethernet-v2-brick`_ - Hardware design files of the WARP ESP32 Ethernet Brick 2.0
- `warp-esp32-ethernet-v2-co-bricklet`_ - Firmware source code of the WARP ESP32 Ethernet 2.0 Co Bricklet (co-processor)
- `evse-v4-bricklet`_ - Hardware design files of the EVSE 4.0 Bricklet (firmware shared with the EVSE 2.0 Bricklet)
- `nfc-bricklet`_ - Firmware source code and hardware design files of the NFC Bricklet

WARP3 Charger
"""""""""""""

- `warp-esp32-ethernet-brick`_ - Hardware design files of the WARP ESP32 Ethernet Brick
- `evse-v3-bricklet`_ - Hardware design files of the EVSE 3.0 Bricklet (firmware shared with the EVSE 2.0 Bricklet)
- `nfc-bricklet`_ - Firmware source code and hardware design files of the NFC Bricklet

WARP2 Charger
"""""""""""""

- `esp32-ethernet-brick`_ - Hardware design files of the ESP32 Ethernet Brick
- `evse-v2-bricklet`_ - Firmware source code and hardware design files of the EVSE 2.0 Bricklet
- `nfc-bricklet`_ - Firmware source code and hardware design files of the NFC Bricklet

WARP1 Charger
"""""""""""""

- `esp32-brick`_ - Hardware design files of the ESP32 Brick
- `evse-bricklet`_ - Firmware source code and hardware design files of the EVSE Bricklet
- `rs485-bricklet`_ - Firmware source code and hardware design files of the RS485 Bricklet

WARP Energy Manager Hardware
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

WARP Energy Manager 2.0
"""""""""""""""""""""""

*Current energy manager generation.*

- `esp32-ethernet-brick`_ - Hardware design files of the ESP32 Ethernet Brick
- `warp-energy-manager-v2-bricklet`_ - Firmware source code and hardware design files of the WARP Energy Manager 2.0 Bricklet
- `warp-front-panel-bricklet`_ - Firmware source code and hardware design files of the WARP Front Panel Bricklet

WARP Energy Manager 1.0
"""""""""""""""""""""""

- `esp32-ethernet-brick`_ - Hardware design files of the ESP32 Ethernet Brick
- `warp-energy-manager-bricklet`_ - Firmware source code and hardware design files of the WARP Energy Manager Bricklet

Forked/patched projects
~~~~~~~~~~~~~~~~~~~~~~~~~

- `arduino-esp32`_
- `esp32-arduino-libs`_
- `WireGuard-ESP32-Arduino`_

.. Link targets: One definition per repository, keep in sync with the lists above.

.. _esp32-firmware: https://github.com/Tinkerforge/esp32-firmware
.. _tfjson: https://github.com/Tinkerforge/tfjson
.. _tfmodbustcp: https://github.com/Tinkerforge/tfmodbustcp
.. _tfocpp: https://github.com/Tinkerforge/tfocpp
.. _tftools: https://github.com/Tinkerforge/tftools
.. _esp32-remote-access: https://github.com/Tinkerforge/esp32-remote-access
.. _warp-charger: https://github.com/Tinkerforge/warp-charger
.. _api.warp-charger.com: https://github.com/Tinkerforge/api.warp-charger.com
.. _vislog.warp-charger.com: https://github.com/Tinkerforge/vislog.warp-charger.com
.. _dbus-warp-charger: https://github.com/Tinkerforge/dbus-warp-charger
.. _warp-esp32-ethernet-v2-brick: https://github.com/Tinkerforge/warp-esp32-ethernet-v2-brick
.. _warp-esp32-ethernet-v2-co-bricklet: https://github.com/Tinkerforge/warp-esp32-ethernet-v2-co-bricklet
.. _evse-v4-bricklet: https://github.com/Tinkerforge/evse-v4-bricklet
.. _nfc-bricklet: https://github.com/Tinkerforge/nfc-bricklet
.. _warp-esp32-ethernet-brick: https://github.com/Tinkerforge/warp-esp32-ethernet-brick
.. _evse-v3-bricklet: https://github.com/Tinkerforge/evse-v3-bricklet
.. _esp32-ethernet-brick: https://github.com/Tinkerforge/esp32-ethernet-brick
.. _evse-v2-bricklet: https://github.com/Tinkerforge/evse-v2-bricklet
.. _esp32-brick: https://github.com/Tinkerforge/esp32-brick
.. _evse-bricklet: https://github.com/Tinkerforge/evse-bricklet
.. _rs485-bricklet: https://github.com/Tinkerforge/rs485-bricklet
.. _warp-energy-manager-v2-bricklet: https://github.com/Tinkerforge/warp-energy-manager-v2-bricklet
.. _warp-front-panel-bricklet: https://github.com/Tinkerforge/warp-front-panel-bricklet
.. _warp-energy-manager-bricklet: https://github.com/Tinkerforge/warp-energy-manager-bricklet
.. _arduino-esp32: https://github.com/Tinkerforge/arduino-esp32
.. _esp32-arduino-libs: https://github.com/Tinkerforge/esp32-arduino-libs
.. _WireGuard-ESP32-Arduino: https://github.com/Tinkerforge/WireGuard-ESP32-Arduino

.. END WARP REPOSITORIES

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
