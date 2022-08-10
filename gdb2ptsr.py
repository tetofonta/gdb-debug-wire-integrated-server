import sys
from binascii import hexlify

def checksum(data):
    return hex(sum([ord(c) for c in data]))[-2:]


if len(sys.argv) > 2 and sys.argv[2] == 'notify':
    data = "O" + hexlify(sys.argv[1].encode()).decode()
else:
    data = sys.argv[1]

msg = f"${data}#{checksum(data)}"
print(msg, len(msg))
