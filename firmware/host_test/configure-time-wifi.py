"""Configure time-recovery Wi-Fi over USB after approved installation.
No credentials are saved on the PC, passed as command-line arguments, or printed.
Requires pyserial. This does not flash firmware or modify aquarium snapshots.
"""
import getpass
import time
import serial
import serial.tools.list_ports


def main():
    ports = [p for p in serial.tools.list_ports.comports()
             if p.serial_number == "10:20:BA:3D:2F:FC"]
    if len(ports) != 1:
        raise SystemExit("Expected Pocket Tank is not connected; nothing changed.")
    ssid = getpass.getpass("Wi-Fi name (hidden): ").encode("utf-8")
    password = getpass.getpass("Wi-Fi password (hidden): ").encode("utf-8")
    if not 1 <= len(ssid) <= 32 or not 8 <= len(password) <= 63 or b"\0" in ssid + password:
        raise SystemExit("Use a 1–32 byte name and an 8–63 byte WPA2/WPA3 password.")
    command = b"clock wifi " + ssid.hex().encode() + b" " + password.hex().encode() + b"\n"
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = ports[0].device, 115200, 0.2
    s.dtr = s.rts = False
    s.open()
    try:
        s.reset_input_buffer()
        s.write(command)
        s.flush()
        del command, ssid, password
        deadline = time.monotonic() + 8
        reply = bytearray()
        while time.monotonic() < deadline:
            reply.extend(s.read(4096))
            if b"clock Wi-Fi setup OK" in reply:
                print("Wi-Fi settings saved. Recovery runs only if the clock is invalid.")
                return
            if b"clock Wi-Fi setup FAILED" in reply:
                raise SystemExit("Settings were not saved; aquarium data is unchanged.")
        raise SystemExit("No setup acknowledgement. Check that the recovery firmware is installed.")
    finally:
        s.close()


if __name__ == "__main__":
    main()
