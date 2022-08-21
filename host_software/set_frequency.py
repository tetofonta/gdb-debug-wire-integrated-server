from binascii import hexlify

from serial import Serial
import argparse
import sys
from _gdb_utils import gdb_write, recv_gdb, get_supported_packet_size


parser = argparse.ArgumentParser(prog=sys.argv[0], description='set remote debugger frequency')
parser.add_argument('--port', dest='port', required=True, help='serial port')
parser.add_argument('--baud', dest='baud', default='115200', type=int, help='serial baud rate')
parser.add_argument('--freq', dest='freq', required=True, type=int, help='serial port')

if __name__ == '__main__':
    args = parser.parse_args()
    with Serial(args.port, args.baud, timeout=1) as ser:
        gdb_write(ser, b'qSupported')
        recv_gdb(ser)
        msg_freq = hexlify((args.freq // 1000).to_bytes(2, 'big'))
        gdb_write(ser, b'qRcmd,' + hexlify(b'frequency ' + msg_freq))
        assert recv_gdb(ser) == b'OK'

