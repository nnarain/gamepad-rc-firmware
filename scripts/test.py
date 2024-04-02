import serial
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument('-p', '--port', required=True)
parser.add_argument('-b', '--baud', default=115200, type=int)

args = parser.parse_args()

port = args.port
baud = args.baud

try:
    while True:
        with serial.Serial(port, baud, timeout=1.0) as ser:
            b = ser.read(10)
            b = list(b)
            b = [int(i) for i in b]
            print(f'{b}')

except KeyboardInterrupt:
    print('exiting')
