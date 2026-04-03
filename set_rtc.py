#!/usr/bin/env python3

import time
import argparse
import serial


def send_and_receive(port, baudrate, message, timeout=1):
    try:
        with serial.Serial(
                port=port,
                baudrate=baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_TWO,
                timeout=timeout
        ) as ser:
            ser.write((message + '\n').encode('ascii', errors='ignore'))
            echo = ser.readline().decode('ascii', errors='ignore').strip()
            # b'\r\r\n'
            ser.readline()
            return echo

    except serial.SerialException as e:
        print(f"Error opening port: {e}")
        return None
    except Exception as e:
        print(f"Error: {e}")
        return None


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'port',
        type=str,
        help='Serial port (example: COM3, /dev/ttyUSB0)'
    )
    args = parser.parse_args()

    PORT = args.port
    BAUDRATE = 115200

    tolerance = 0.01
    target_fraction = 0
    while True:
      now = time.time()
      fraction = now - int(now)
        
      if abs(fraction - target_fraction) <= tolerance:
        break;
      else:
        sleep_time = (target_fraction - fraction) * 0.8
        if sleep_time > 0:
          time.sleep(sleep_time)

    command = "set_rtc %d" % time.time()
    print(time.time())
    response = send_and_receive(PORT, BAUDRATE, command)

    if response.strip() == command.strip():
        print(response)
        print("Success")
    else:
        print("Failed")
