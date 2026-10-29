# Example G: SDI-12 Terminal and PC Interface<!--! {#example_g_page} -->

This example uses an Arduino as an interactive SDI-12 command terminal, a
passive bus monitor, or an interface between PC software and an SDI-12 sensor.
It replaces the deprecated `g_terminal_window` and `i_SDI-12_interface`
examples.

The sketch continuously listens to the SDI-12 bus and forwards each received
character to the serial port once. Entering a line in the serial terminal sends
it as an SDI-12 command. CR, LF, and CRLF serial line endings are supported.

In the default SDI-12 command mode, the sketch adds `!` only when it is missing;
all other characters, including lowercase sensor addresses, remain unchanged.
Use `mode v` to send entered text verbatim. Use `fb off` after selecting verbatim
mode for a quiet software interface without input echo or interface messages.
The serial line ending terminates the PC message and is not transmitted on the
SDI-12 bus.

Because the Arduino remains in listening mode between commands, the sketch can
also monitor communication between another data recorder and its sensors. Do
not enter commands while using it only as a passive monitor.

<!--! @section g_terminal_interface_pio PlatformIO Configuration -->

<!--! @include{lineno} g_terminal_interface/platformio.ini -->

<!--! @section g_terminal_interface_code The Complete Example -->

<!--! @include{lineno} g_terminal_interface/g_terminal_interface.ino -->
