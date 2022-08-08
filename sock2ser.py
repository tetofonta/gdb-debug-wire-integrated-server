import os
import threading

from serial import Serial
import socket
import signal

threads = []
stop = False
sock = None
ser = None


def thread_from_dev():
    try:
        while not stop:
            data = ser.read(1024)
            if len(data) > 0:
                print("->", data, ":", len(data))
                sock.send(data)
    except Exception as e:
        print("-> EXITED", e)
        stop_session()


def thread_to_dev():
    try:
        while not stop:
            data = sock.recv(1024)
            if len(data) > 0:
                print("<-", data, ":", len(data))
                ser.write(data)
    except Exception as e:
        print("<- EXITED", e)
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
            f = threading.Thread(target=thread_from_dev)
            t = threading.Thread(target=thread_to_dev)
            f.start()
            t.start()
            threads.extend([t])

