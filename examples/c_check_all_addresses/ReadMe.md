# Example C: Checks all addresses for active sensors, and prints their status to the serial port.

This is a simple demonstration of the SDI-12 library for Arduino.

It discovers the address of all sensors active on any pin on your board.

Each sensor should have a unique address already - if not, multiple sensors may respond simultaenously to the same request and the output will not be readable by the Arduino.

To address a sensor, please see Example B: b_address_change.ino
