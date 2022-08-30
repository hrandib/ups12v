#! /usr/bin/python

import serial



ser = serial.Serial('/dev/ttyACM0', timeout=5)
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
