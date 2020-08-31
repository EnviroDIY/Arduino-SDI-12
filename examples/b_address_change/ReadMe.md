[//]: # ( @page example_b_page Example B: Changing the Address of your SDI-12 Sensor )
# Example B: Changing the Address of your SDI-12 Sensor

Communication with an SDI-12 sensor depends on its 1-character alphanumeric address (1-9, A-Z, a-z).  A sensor can also be programmed with an address of 0, but that address cannot always be used to get measurements from the sensor.  This sketch enables you to find and change the address of your sensor.

First, physically connect your SDI-12 sensor to your device.  Some helpful hits for connecting it can be found here:  https://envirodiy.org/topic/logging-mayfly-with-decagon-sdi-12-sensor/#post-2129.

Once your sensor is physically connected to your board, download this library and open this sketch.

Scroll to line 54 of the sketch (`#define DATA_PIN 7`).  Change the `7` to the pin number that your sensor is attached to.

Set the pin to provide power to your sensor in line 55 (`#define POWER_PIN 22`).  If your sensor is continuously powered, set the power pin to -1.

Upload the sketch to your board.  After the upload finishes, open up the serial port monitor at a baud rate of 115200 on line 53.

In the serial monitor you will see it begin scanning through all possible SDI-12 addresses.  Once it has found an occupied address, it will stop and ask you to enter a new address.  Send your desired address to the serial port.  On the screen you should see "Readdressing Sensor." followed by "Success.  Rescanning for verification."  The scan will begin again, stopping at your new address.  If you are now happy with the address you've selected, smile and close the serial port monitor.

If you are using a Meter Group Hydros 21 CTD sensor, change the channel to 1 in the serial monitor where prompted.

[//]: # ( @section b_address_change_pio PlatformIO Configuration )

[//]: # ( @include{lineno} b_address_change/platformio.ini )

[//]: # ( @section b_address_change_code The Complete Example )
