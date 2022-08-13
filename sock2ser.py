import os
import threading
from binascii import hexlify

from serial import Serial
import socket
import signal

threads = []
stop = False
sock = None
ser = None

last = True


def thread(dir_str, read_fn, send_fn, last_v):
    global last
    line = False
    try:
        while not stop:
            data = read_fn(1024)
            if len(data) > 0:
                if last != last_v:
                    last = last_v
                    print(f"\n{dir_str} ", end='')
                try:
                    print(data.decode(), end='')
                except:
                    print(f"[{hexlify(data).decode()}]", end='')
                send_fn(data)
    except Exception as e:
        print("-> EXITED", e)
        stop_session()


def stop_session(*args, **kwargs):
    global stop, ser, sock, threads
    print("Terminating session...")
    stop = True
    for tr in threads:
        tr.join(1)
    threads = []
    print("OK")
    if sock is not None:
        sock.close()
    if ser is not None:
        ser.close()
    stop = False


if __name__ == '__main__':
    signal.signal(signal.SIGTSTP, stop_session)
    if os.path.exists("/tmp/ttyV0"):
        os.remove("/tmp/ttyV0")
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as server:
        server.bind("/tmp/ttyV0")
        while True:
            print("waiting")
            server.listen(1)
            sock, addr = server.accept()
            ser = Serial('/dev/ttyACM0', 250000, timeout=0)
            f = threading.Thread(target=thread, args=('->', lambda x: ser.read(x), lambda d: sock.send(d), True))
            t = threading.Thread(target=thread, args=('<-', lambda x: sock.recv(x), lambda d: ser.write(d), False))
            f.start()
            t.start()
            threads.extend([t])

