# Project for Testing Built-in RTC of nRF52840

**Board used:** Pro Micro nRF52840  
**SoftDevice used:** s140_nrf52_6.1.1

## Pin Assignment
- **UART TX** – P0.17  
- **UART RX** – P0.20  

**UART baudrate:** 115200

Commands are entered as text. Available commands:

1) `set_rtc <number>`  
   Set the timestamp value (32-bit).  
   Example: `set_rtc 100`

2) `rtc`  
   Read the timestamp.  
   Example response: `RTC: 104`

## Scripts

**set_rtc.py**  
Sets `set_rtc` according to the current PC time.  
Argument: port  
Example: `./set_rtc.py /dev/ttyUSB0`

**get_rtc.py**  
   Reads the RTC and calculates the difference between the read value and the current PC time.  
   Argument: port  
   Example: `./get_rtc.py /dev/ttyUSB0`  
   Output:  
   Timestamp: 1775214113  
   RTC is behind by: 0.21336913108825684 seconds  
   Date: 2026-04-03  
   Time: 14:01:53
