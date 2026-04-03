#!/usr/bin/env python3

import time
from datetime import datetime
import argparse
import serial
import re


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
            response = ser.readline().decode('ascii', errors='ignore').strip()
            return response

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

    command = "rtc"
    response = send_and_receive(PORT, BAUDRATE, command)

    if response is not None:
        match = re.match(r'^RTC:\s+(\d+)$', response.strip())
        if match:
            timestamp = int(match.group(1))
            readable_time = datetime.fromtimestamp(timestamp)

            print(f"Timestamp: {timestamp}")
            difference = abs(time.time() - timestamp)
            if time.time() > timestamp:
                print(f"RTC is behind by: {difference} seconds")
            else:
                print(f"RTC is ahead by: {difference} seconds")
            print(f"Date: {readable_time.strftime('%Y-%m-%d')}")
            print(f"Time: {readable_time.strftime('%H:%M:%S')}")
