#! /usr/bin/python

import serial
import configparser

conf = configparser.ConfigParser()
conf.read('ups.conf')

ser = serial.Serial('/dev/' + conf.get('UPS', 'Tty', fallback='ttyACM0'), timeout=5)


def send_command(*cmd_with_args):
    cmd = ' '.join(map(str, cmd_with_args))
    ser.write(b'\x03\r')
    ser.reset_input_buffer()
    ser.write(bytes(f'{cmd}\r', 'utf8'))
    ser.readline()
    ser.readline()
    return ser.readline()[:-2].decode('utf8')


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


set_limits()
exit(0)

ser.write(b'\x03\r')
ser.reset_input_buffer()
ser.write(b'poll\r')
while len(ser.readline().split()) != 4:
    pass
while True:
    v12, vbat, diff, status = ser.readline().split()
    v12 = int(v12)
    vbat = int(vbat)
    diff = int(diff)
    status = status.decode('utf8')
    print(v12, vbat, diff, status)
ser.close()
