#! /usr/bin/python

import serial
import configparser
import signal

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
    print("Limit charge >", end=' ')
    if conf.has_option('UPS', 'LimitCharge'):
        res = send_command('limit-charge', ups['LimitCharge'])
        print(res)
    else:
        print("As is, no overwrite")
    print("Limit idle discharge >", end=' ')
    if conf.has_option('UPS', 'LimitIdleDischarge'):
        res = send_command('limit-discharge', ups['LimitIdleDischarge'])
        print(res)
    else:
        print("As is, no overwrite")


print(send_command("info"))
set_limits()

shutdown_threshold = conf.getint('UPS', 'ShutdownThreshold', fallback=30)
print(f'Shutdown will be initiated reaching {shutdown_threshold}% battery level during discharge')


def shutdown():
    print('Shutdown started')


send_command('poll')
ser.timeout = 2
while True:
    v12, vbat, diff, percent, status = ser.readline().split()
    v12 = int(v12)
    vbat = int(vbat)
    diff = int(diff)
    percent = int(percent[:-1])
    status = status.decode('utf8')
    if status == 'DISCHARGE' and percent <= shutdown_threshold:
        shutdown()
    print(v12, vbat, diff, percent, status)
