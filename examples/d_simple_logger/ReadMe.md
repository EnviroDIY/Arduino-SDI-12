[//]: # ( @page example_d_page Example D: Check all Addresses for Active Sensors and Log Data )
## Example D: Check all Addresses for Active Sensors and Log Data

This is a simple demonstration of the SDI-12 library for Arduino.

It discovers the address of all sensors active on a single bus and takes measurements from them.

Every SDI-12 device is different in the time it takes to take a measurement, and the amount of data it returns.  This sketch will not serve every sensor type, but it will likely be helpful in getting you started.

Each sensor should have a unique address already - if not, multiple sensors may respond simultaneously to the same request and the output will not be readable by the Arduino.

To address a sensor, please see Example B: b_address_change.ino

[//]: # ( @section d_simple_logger_pio PlatformIO Configuration )

[//]: # ( @include{lineno} d_simple_logger/platformio.ini )

[//]: # ( @section d_simple_logger_code The Complete Example )
