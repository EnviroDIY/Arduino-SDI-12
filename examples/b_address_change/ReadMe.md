# Example B: Changing the Address of your SDI-12 Sensor<!--! {#example_b_page} -->

Communication with an SDI-12 sensor depends on its one-character alphanumeric
address (`0`-`9`, `a`-`z`, or `A`-`Z`). This sketch uses the blocking
`SDI12Recorder` API to find a sensor, print its identification information, and
change its address with a validated acknowledgement.

First, physically connect your SDI-12 sensor to your device. Some helpful hints are
available in this [EnviroDIY forum discussion](https://envirodiy.org/topic/logging-mayfly-with-decagon-sdi-12-sensor/#post-2129).

Once your sensor is physically connected to your board, download this library and open this sketch.

Scroll to line 17 of the sketch (`#define SDI12_DATA_PIN 7`).  Change the `7` to the pin number that your sensor is attached to.
You could also define the data pin in your platformio.ini: (`-D SDI12_DATA_PIN=##`)

Set the pin to provide power to your sensor in line 20 (`#define SDI12_POWER_PIN 22`).  If your sensor is continuously powered, set the power pin to -1.

Upload the sketch to your board. After the upload finishes, open the serial port
monitor at 115200 baud, or at the baud rate configured by
`SDI12_TERMINAL_BAUD`.

The sketch assumes exactly one sensor is attached. It queries that sensor's current
address and asks you to enter a new address. Send your desired address to the serial
port. After the sensor acknowledges the change, the sketch queries the bus again to
verify the new address.

If you are using a Meter Group Hydros 21 CTD sensor, change the channel to 1 in the serial monitor where prompted.

<!--! @section b_address_change_pio PlatformIO Configuration -->

<!--! @include{lineno} b_address_change/platformio.ini -->

<!--! @section b_address_change_code The Complete Example -->

<!--! @include{lineno} b_address_change/b_address_change.ino -->
