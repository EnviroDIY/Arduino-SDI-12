# Example I:  SDI-12 PC Interface

Code for an Arduino-based USB dongle translates serial comm from PC to SDI-12 (electrical and timing)
1. Allows user to communicate to SDI-12 devices from a serial terminal emulator (e.g. PuTTY).
2. Able to spy on an SDI-12 bus for troubleshooting comm between datalogger and sensors.
3. Can also be used as a hardware middleman for interfacing software to an SDI-12 sensor.  For example, implementing an SDI-12 datalogger in Python on a PC.  Use verbatim mode with feedback off in this case.

Note: "translation" means timing and electrical interface.  It does not ensure SDI-12 compliance of commands sent via it.
