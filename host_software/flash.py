import argparse
import sys
import os
from binascii import hexlify, unhexlify
from tqdm import tqdm
from serial import Serial
import intelhex
from _gdb_utils import gdb_write, recv_gdb, get_supported_packet_size

parser = argparse.ArgumentParser(prog=sys.argv[0], description='Flash files to the target using gdb server')
parser.add_argument('--flash', dest='flash_file', help='flash file to be written')
parser.add_argument('--eeprom', dest='eeprom_file', help='eeprom file to be written')
parser.add_argument('--baud', dest='baud', default='115200', type=int, help='serial baud rate')
parser.add_argument('--port', dest='port', required=True, help='serial port')
parser.add_argument('--mcu', dest='mcu', required=True, help='MCU name')
parser.add_argument('--no-verify', dest='noverify', action='store_true', help='If present, no verification after write is performed')

SIGNATURES = {
    'm328p': "950f",
    'atmega328p': "950f"
}


def write_file(file: str, address: int, packet_size: int, verify: bool, ser: Serial):
    ih = intelhex.IntelHex()
    ih.loadfile(file, 'hex' if file.endswith('.hex') else 'bin')

    packet_size_characters = "M{:x},{:x}:".format(address + len(ih), packet_size)
    packet_size -= len(packet_size_characters)

    for addr in tqdm(range(address, address + len(ih), packet_size//2)):
        wrt = hexlify(ih.tobinstr(start=addr, size=packet_size//2))
        s = "M{:x},{:x}:".format(addr, packet_size//2).encode() + wrt
        gdb_write(ser, s)
        assert recv_gdb(ser) == b'OK'

    gdb_write(ser, b'qRcmd,7265')
    assert recv_gdb(ser) == b'OK'

    if verify:
        print("Verifying...")
        for addr in tqdm(range(address, address + len(ih), packet_size//2)):
            gdb_write(ser, "m{:x},{:x}".format(addr, packet_size//2).encode())
            assert hexlify(ih.tobinstr(start=addr, size=packet_size//2)) == recv_gdb(ser)


def recv_signature(packet: bytes):
    packet = packet.decode()
    assert packet[0] == 'O'
    return unhexlify(packet[1:-2]).decode()


if __name__ == '__main__':
    args = parser.parse_args()
    if args.flash_file is None and args.eeprom_file is None:
        parser.print_help()
        exit(1)

    with Serial(args.port, args.baud, timeout=1) as ser:
        gdb_write(ser, b'qSupported')
        packet_size = get_supported_packet_size(recv_gdb(ser))
        gdb_write(ser, b'?')
        signal = recv_gdb(ser)
        if signal == b'S01':
            print("Unable to halt the target.")
            exit(1)

        gdb_write(ser, b'qRcmd,73')
        signature = recv_signature(recv_gdb(ser))
        assert recv_gdb(ser) == b'OK'

        if args.mcu not in SIGNATURES:
            print(f"Unknown device with name {args.mcu}")
            print("Available", "\n".join(SIGNATURES.keys()))

        if SIGNATURES[args.mcu] != signature:
            print(f"Not the expected MCU: Expected {SIGNATURES[args.mcu]}, found {signature}")
            exit(1)

        if args.flash_file is not None and os.path.exists(args.flash_file) and os.path.isfile(args.flash_file):
            print(f"Writing flash from {args.flash_file}")
            write_file(args.flash_file, 0, packet_size, not args.noverify, ser)

        if args.eeprom_file is not None and os.path.exists(args.eeprom_file) and os.path.isfile(args.eeprom_file):
            print(f"Writing eeprom from {args.eeprom_file}")
            write_file(args.eeprom_file, 0x810000, packet_size, not args.noverify, ser)

        gdb_write(ser, b'D')
        print("ok")
