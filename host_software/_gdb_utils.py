from serial import Serial


def recv_gdb(ser: Serial):
    buf = b''
    a = ser.read(1)
    while a != b'$':
        a = ser.read(1)
    while len(buf) == 0 or buf[len(buf) - 1] != ord(b'#'):
        buf += ser.read(1)
    buf = buf[:-1]
    if int(ser.read(2), 16) != sum(buf) & 0xFF:
        raise Exception()
    return buf


def gdb_write(ser: Serial, message: bytes):
    checksum = "{:02x}".format(sum(message) & 0xFF).encode()
    ser.write(b'+$' + message + b'#' + checksum)


def get_supported_packet_size(resp):
    ps = 32
    resp = resp.split(b';')
    for r in resp:
        if r.startswith(b'PacketSize'):
            ps = int(r.split(b'=')[1], 16)
    return ps
