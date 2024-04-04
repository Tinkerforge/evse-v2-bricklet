#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import sys
import subprocess
import os

from evse_v3_tester import EVSEV3Tester


TEST_LOG_FILENAME = "full_test_log.csv"
TEST_LOG_DIRECTORY = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', '..', 'wallbox', 'evse_v3_test_report'))

def test_log_pull():
    try:
        inp = ['git', 'pull']
        out = subprocess.check_output(inp , stderr=subprocess.STDOUT, encoding='utf-8', cwd=TEST_LOG_DIRECTORY)
        print('   -> {}'.format(' '.join(inp)))
        print('      {}'.format(out))
    except subprocess.CalledProcessError as e:
        print('Error: git pull failed:\n' + e.output.strip())
    except Exception as e:
        print('Error: git pull failed:\n' + str(e))

def test_log_commit_and_push(uid):
    commit_message = 'Add test report for EVSE Bricklet 3.0 with UID ' + uid
    test_log_pull()

    try:
        inp = ['git', 'commit', TEST_LOG_FILENAME, '-m', commit_message]
        out = subprocess.check_output(inp, stderr=subprocess.STDOUT, encoding='utf-8', cwd=TEST_LOG_DIRECTORY)
        print('   -> {}'.format(' '.join(inp)))
        print('      {}'.format(out))
    except subprocess.CalledProcessError as e:
        print('Error: git commit failed:\n' + e.output.strip())
    except Exception as e:
        print('Error: git commit failed:\n' + str(e))

    try:
        inp = ['git', 'push']
        out = subprocess.check_output(inp, stderr=subprocess.STDOUT, encoding='utf-8', cwd=TEST_LOG_DIRECTORY)
        print('   -> {}'.format(' '.join(inp)))
        print('      {}'.format(out))
    except subprocess.CalledProcessError as e:
        print('Error: git push failed:\n' + e.output.strip())
    except Exception as e:
        print('Error: git push failed:\n' + str(e))


def no_log(s):
    pass

def test_value(value, expected, margin_percent=0.1, margin_absolute=20):
    if value < 0 and expected > 0:
        return False
    if value > 0 and expected < 0:
        return False

    value = abs(value)
    expected = abs(expected)

    return (value*(1-margin_percent) - margin_absolute) < expected < (value*(1+margin_percent) + margin_absolute)

def main():
    test_log_pull()
    print('Schaltereinstellung auf 32A stellen (1=Off, 2=Off, 3=On, 4=On) !!!')

    print('Suche EVSE Bricklet 3.0 und Tester')
    evse_tester = EVSEV3Tester(log_func = no_log)
    evse_tester.set_led(0, 0, 255)
    print('... OK')

    print('Prüfe Hardware-Version (erwarte V3)')
    hv = evse_tester.get_hardware_version()
    if hv == 30:
        print('...OK')
    else:
        print('-----------------> NICHT OK: {0}'.format(hv))
        evse_tester.exit(1)

    data = []

    ident = evse_tester.evse.get_identity()
    data.append(ident.uid)

    # Initial config
    evse_tester.set_contactor_fb(False)
    evse_tester.set_230v(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()

    print('Warte auf DC-Schutz Kalibrierung (1.5 Sekunden)')
    time.sleep(1.2)
    print('... OK')

    hw_conf = evse_tester.evse.get_hardware_configuration()
    print('Teste Schalter-Einstellung')
    if hw_conf.jumper_configuration != 6:
        print('Falsche Schalter-Einstellung: {0}'.format(hw_conf.jumper_configuration))
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste Lock-Switch-Einstellung')
    if hw_conf.has_lock_switch:
        print('Falsche Lock-Switch-Einstellung: {0}'.format(hw_conf.has_lock_switch))
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste Shutdown Input high')
    evse_tester.shutdown_input_enable(True)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[18]
    if value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste Shutdown Input low')
    evse_tester.shutdown_input_enable(False)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[18]
    if not value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste CP/PE...')
    print(' * open')
    evse_tester.set_max_charging_current(0)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if res_cppe == 4294967295:
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0} Ohm'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 12213):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0} mV (erwartet 12213 mV)'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 2700 Ohm')
    evse_tester.set_cp_pe_resistor(True, False, False)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 2700):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 2700 Ohm)'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 9069):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0} mV (erwartet 9069 mV)'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 880 Ohm')
    evse_tester.set_cp_pe_resistor(True, True, False)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 880):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 880 Ohm)'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 6049):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0} mv (erwartet 6049 mV)'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 240 Ohm')
    evse_tester.set_cp_pe_resistor(True, True, True)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 240):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 240 Ohm)'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 2646):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0} mV (erwartet 2646 mV)'.format(vol_cppe))
        evse_tester.exit(1)

    evse_tester.set_cp_pe_resistor(False, False, False)
    time.sleep(0.5)


    print('Teste PP/PE...')
    print(' * 220 Ohm')
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 220):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 220 Ohm)'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 834):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0} mv (erwartet 834 mV)'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 1500 Ohm')
    evse_tester.set_pp_pe_resistor(True, False, False, False)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 1500):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 1500 Ohm)'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 2313):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0} mv (erwartet 2313 mV)'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 680 Ohm')
    evse_tester.set_pp_pe_resistor(False, True, False, False)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 680):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 680 Ohm)'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 1695):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0} mV (erwartet 1695 mV)'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 100 Ohm')
    evse_tester.set_pp_pe_resistor(False, False, False, True)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 100):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0} Ohm (erwartet 100 Ohm)'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 441):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0} mV (erwartet 441 mV)'.format(vol_pppe))
        evse_tester.exit(1)

    evse_tester.set_pp_pe_resistor(False, False, True, False)
    time.sleep(0.5)


    print('Beginne Test-Ladung')

    evse_tester.set_contactor_fb(False)
    evse_tester.set_230v(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()
    time.sleep(0.5)

    print('Setze 2700 Ohm + 1300 Ohm Widerstand')
    evse_tester.set_cp_pe_resistor(True, True, False)
    print('... OK')

    evse_tester.wait_for_contactor_gpio(False)

    print('Aktiviere Schütz (2 Sekunden)')
    evse_tester.set_contactor_fb(True)

    time.sleep(0.5)
    print('... OK')

    test_voltages = [-10302, -9698, -9095, -8476, -7871, -7272, -6648, -6047, -5445, -4822, -4224, -3617, -3000, -2396]
    for i, a in enumerate(range(6, 33, 2)):
        print('Test CP/PE {0}A'.format(a))
        evse_tester.set_max_charging_current(a*1000)
        time.sleep(0.5)
        res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
        data.append(str(res_cppe))
        if test_value(res_cppe, 880):
            print(' * ... OK ({0} Ohm)'.format(res_cppe))
        else:
            print('-----------------> NICHT OK {0} Ohm (erwartet 880 Ohm)'.format(res_cppe))
            evse_tester.exit(1)
        vol_cppe = evse_tester.get_cp_pe_voltage()
        if test_value(vol_cppe, test_voltages[i], margin_percent=0.15):
            print(' * ... OK ({0} mV)'.format(vol_cppe))
        else:
            print('-----------------> NICHT OK {0} mV (erwartet {1} mV)'.format(vol_cppe, test_voltages[i]))
            evse_tester.exit(1)

    print('Teste Stromzähler')
    values, detailed_values, hw, error = evse_tester.get_energy_meter_data()
    if (not hw.energy_meter_type > 0) or (not values.phases_connected[0]):
        print('-----------------> NICHT OK: {0}, {1}, {2}'.format(str(hw), str(error), str(values)))
        evse_tester.exit(1)
    else:
        print('... OK: {0}, {1}'.format(hw.energy_meter_type, values.phases_connected[0]))


    print('Ausschaltzeit messen')
    t1 = time.time()
    evse_tester.set_cp_pe_resistor(True, False, False)
    evse_tester.wait_for_contactor_gpio(True)
    evse_tester.set_contactor_fb(False)
    t2 = time.time()

    delay = int((t2-t1)*1000)
    data.append(str(delay))
    print('... OK')

    if delay <= 100:
        print('Ausschaltzeit: {0}ms OK'.format(delay))
    else:
        print('Ausschaltzeit: {0}ms'.format(delay))
        print('-----------------> NICHT OK')
        evse_tester.exit(1)

    print('Teste Front-Taster')
    evse_tester.press_button(True)

    evse_tester.wait_for_button_gpio(True) # Button True = Pressed
    print('... OK')
    evse_tester.press_button(False)

    print('Teste LED R')
    evse_tester.set_evse_led(True, False, False)
    time.sleep(1)
    led = evse_tester.get_evse_led()
    if led[0] and (not led[1]) and (not led[2]):
        print('... OK')
    else:
        print('-----------------> NICHT OK: {0} {1} {2}'.format(*led))
        evse_tester.exit(1)
    print('Teste LED G')
    evse_tester.set_evse_led(False, True, False)
    time.sleep(1)
    led = evse_tester.get_evse_led()
    if led[1] and (not led[0]) and (not led[2]):
        print('... OK')
    else:
        print('-----------------> NICHT OK: {0} {1} {2}'.format(*led))
        evse_tester.exit(1)
    print('Teste LED B')
    evse_tester.set_evse_led(False, False, True)
    time.sleep(1)
    led = evse_tester.get_evse_led()
    if led[2] and (not led[0]) and (not led[1]):
        print('... OK')
    else:
        print('-----------------> NICHT OK: {0} {1} {2}'.format(*led))
        evse_tester.exit(1)

    print('')
    print('Fertig. Alles OK')

    with open(os.path.join(TEST_LOG_DIRECTORY, TEST_LOG_FILENAME), 'a+') as f:
        f.write(', '.join(data) + '\n')

    test_log_commit_and_push(ident.uid)

    evse_tester.exit(0)

if __name__ == "__main__":
    main()
