#! /usr/bin/python

import serial
import configparser
import signal
import os

conf = configparser.ConfigParser()
conf.read('ups.conf')

ser = serial.Serial('/dev/' + conf.get('UPS', 'Tty', fallback='ttyACM0'), timeout=0.05)
MAX_INPUT_LEN = 512


def signal_handler(sig, frame):
    print(' SIGINT captured')
    exit(0)


signal.signal(signal.SIGINT, signal_handler)


def port_is_alive():
    try:
        ser.inWaiting()
        return True
    except serial.SerialException:
        return False


def send_command(*cmd_with_args):
    if not port_is_alive():
        exit(-1)
    cmd = ' '.join(map(str, cmd_with_args))
    ser.write(b'\x03\r')
    ser.reset_input_buffer()
    ser.write(bytes(f'{cmd}\r', 'utf8'))
    ser.readline()
    ser.readline()
    output = ser.read(MAX_INPUT_LEN).decode('utf8')
    output = output[0:output.rfind('\r\n')]
    return output


def set_limits():
    ups = conf['UPS']
    if conf.has_option('UPS', 'LimitCharge'):
        val = ups['LimitCharge']
        print(f"Limit charge to {val}% >", end=' ')
        res = send_command('limit-charge', val)
        print(res)
    if conf.has_option('UPS', 'LimitIdleDischarge'):
        val = ups['LimitIdleDischarge']
        print(f"Limit idle discharge {val}% >", end=' ')
        res = send_command('limit-discharge', val)
        print(res)


print(send_command("info"))
set_limits()

shutdown_threshold = conf.getint('UPS', 'ShutdownThreshold', fallback=30)
print(f'Shutdown will be initiated reaching {shutdown_threshold}% battery level during discharge')

shutdown_triggered = False


def shutdown():
    global shutdown_triggered
    if not shutdown_triggered:
        print('Shutdown initiated')
        shutdown_triggered = True
        os.system('shutdown +1')


def cancel_shutdown():
    global shutdown_triggered
    print('Shutdown canceling')
    shutdown_triggered = False
    os.system('shutdown -c')


def log_db(v12, vbat, diff, level, state):
    print(f'To DB: {v12} {vbat} {diff} {level} {state}')


def log_dummy(*args):
    pass


log = log_db if conf.has_option('INFLUXDB', "Host") else log_dummy

print(send_command('poll'))
ser.timeout = 2
prev_level = 0
while True:
    v12, vbat, diff, percent, state = ser.readline().split()
    v12 = int(v12)
    vbat = int(vbat)
    diff = int(diff)
    level = int(percent[:-1])
    state = state.decode('utf8')
    if (state in ['DISCHARGE', 'IDLE'] and prev_level > level) \
            or (state in ['CHARGE', 'TRICKLE'] and prev_level < level):
        prev_level = level
        log(v12, vbat, diff, level, state)
        if level % 5 == 0:
            print(f'State: {state} {level}%')  # system log
    if state == 'DISCHARGE' and level <= shutdown_threshold:
        shutdown()
    if state == 'CHARGE' and shutdown_triggered:
        cancel_shutdown()

