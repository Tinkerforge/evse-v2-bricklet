#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import sys
import subprocess
import os

from evse_v2_tester import EVSEV2Tester


TEST_LOG_FILENAME = "full_test_log.csv"
TEST_LOG_DIRECTORY = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', '..', 'wallbox', 'evse_v2_test_report'))

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
    commit_message = 'Add test report for EVSE Bricklet 2.0 with UID ' + uid
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

if __name__ == "__main__":
    test_log_pull()
    print('Schaltereinstellung auf 32A stellen (mitte, rechts)')
    input("Enter drücken...")
    print('')

    print('Suche EVSE Bricklet 2.0 und Tester')
    evse_tester = EVSEV2Tester(log_func = no_log)
    print('... OK')

    data = []

    ident = evse_tester.evse.get_identity()
    data.append(ident.uid)

    # Initial config
    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()

    print('Warte auf DC-Schutz Kalibrierung (1.5 Sekunden)')
    print('--> Flackert LED? Wenn nicht kaputt! <--')
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


    print('Teste GPIO Input/Output low')
    evse_tester.evse.set_gpio_configuration(0, 0, 0)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[16]
    if value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste GPIO Input/Output high')
    evse_tester.evse.set_gpio_configuration(0, 0, 1)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[16]
    if not value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste Shutdown Input high')
    evse_tester.shutdown_input_enable(True)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[5]
    if value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste Shutdown Input low')
    evse_tester.shutdown_input_enable(False)
    time.sleep(0.1)
    value = evse_tester.evse.get_low_level_state().gpio[5]
    if not value:
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Teste CP/PE...')
    print(' * open')
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
        print('-----------------> NICHT OK {0} mV'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 2700 Ohm')
    evse_tester.set_cp_pe_resistor(True, False, False)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 2700):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 9069):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 880 Ohm')
    evse_tester.set_cp_pe_resistor(True, True, False)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 880):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 6049):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_cppe))
        evse_tester.exit(1)

    print(' * 240 Ohm')
    evse_tester.set_cp_pe_resistor(True, True, True)
    time.sleep(0.5)
    res_cppe = evse_tester.evse.get_low_level_state().resistances[0]
    data.append(str(res_cppe))
    if test_value(res_cppe, 240):
        print(' * ... OK ({0} Ohm)'.format(res_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_cppe))
        evse_tester.exit(1)
    vol_cppe = evse_tester.get_cp_pe_voltage()
    if test_value(vol_cppe, 2646):
        print(' * ... OK ({0} mV)'.format(vol_cppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_cppe))
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
        print('-----------------> NICHT OK {0}'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 834):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 1500 Ohm')
    evse_tester.set_pp_pe_resistor(True, False, False, False)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 1500):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 2313):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 680 Ohm')
    evse_tester.set_pp_pe_resistor(False, True, False, False)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 680):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 1695):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_pppe))
        evse_tester.exit(1)

    print(' * 100 Ohm')
    evse_tester.set_pp_pe_resistor(False, False, False, True)
    time.sleep(0.5)
    res_pppe = evse_tester.evse.get_low_level_state().resistances[1]
    data.append(str(res_pppe))
    if test_value(res_pppe, 100):
        print(' * ... OK ({0} Ohm)'.format(res_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(res_pppe))
        evse_tester.exit(1)
    vol_pppe = evse_tester.get_pp_pe_voltage()
    if test_value(vol_pppe, 441):
        print(' * ... OK ({0} mV)'.format(vol_pppe))
    else:
        print('-----------------> NICHT OK {0}'.format(vol_pppe))
        evse_tester.exit(1)

    evse_tester.set_pp_pe_resistor(False, False, True, False)
    time.sleep(0.5)


    print('Beginne Test-Ladung')

    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()
    time.sleep(0.5)

    print('Setze 2700 Ohm + 1300 Ohm Widerstand')
    evse_tester.set_cp_pe_resistor(True, True, False)
    print('... OK')

    evse_tester.wait_for_contactor_gpio(False)

    print('Aktiviere Schütz (2 Sekunden)')
    evse_tester.set_contactor(True, True)

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
            print('-----------------> NICHT OK {0}'.format(res_cppe))
            evse_tester.exit(1)
        vol_cppe = evse_tester.get_cp_pe_voltage()
        if test_value(vol_cppe, test_voltages[i]):
            print(' * ... OK ({0} mV)'.format(vol_cppe))
        else:
            print('-----------------> NICHT OK {0}'.format(vol_cppe))
            evse_tester.exit(1)

    print('Teste Stromzähler')
    values, detailed_values, hw, error = evse_tester.get_energy_meter_data()
    if (not hw.energy_meter_type > 0) or (values.energy_absolute < 1):
        print('-----------------> NICHT OK: {0}, {1}, {2}'.format(str(hw), str(error), str(values)))
        evse_tester.exit(1)
    else:
        print('... OK: {0}, {1}'.format(hw.energy_meter_type, values.energy_absolute))


    print('Ausschaltzeit messen')
    t1 = time.time()
    evse_tester.set_cp_pe_resistor(True, False, False)
    evse_tester.wait_for_contactor_gpio(True)
    evse_tester.set_contactor(True, False)
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

    print('Teste Schalter-Einstellung')

    print('Schaltereinstellung auf "20A" stellen und dann Taster drücken (rechts, mitte)')
    evse_tester.wait_for_button_gpio(True) # Button True = Pressed
    print('')

    evse_tester.evse.reset()
    time.sleep(0.5)
    hw_conf = evse_tester.evse.get_hardware_configuration()
    if hw_conf.jumper_configuration != 4:
        print('Falsche Schalter-Einstellung: {0}'.format(hw_conf.jumper_configuration))
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('Schaltereinstellung auf "Disabled" stellen und dann Taster drücken (links, links)')
    evse_tester.wait_for_button_gpio(True) # Button True = Pressed
    print('')

    evse_tester.evse.reset()
    time.sleep(0.5)
    hw_conf = evse_tester.evse.get_hardware_configuration()
    if hw_conf.jumper_configuration != 8:
        print('Falsche Schalter-Einstellung: {0}'.format(hw_conf.jumper_configuration))
        print('-----------------> NICHT OK')
        evse_tester.exit(1)
    else:
        print('... OK')

    print('')
    print('Fertig. Alles OK')

    with open(os.path.join(TEST_LOG_DIRECTORY, TEST_LOG_FILENAME), 'a+') as f:
        f.write(', '.join(data) + '\n')

    test_log_commit_and_push(ident.uid)

    evse_tester.exit(0)
