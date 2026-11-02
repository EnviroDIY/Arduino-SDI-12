# Raw Extended Commands<!--! {#raw_extended_commands_page} -->

This advanced example sends one manufacturer-specific command with
`SDI12Recorder::startRawCommand()`. Replace the example command body with the
value documented by the sensor manufacturer. Do not include the sensor address
or terminating exclamation point.

The raw path validates the response address and retry timing, but it does not
interpret the remaining payload or validate a command-specific CRC. It also
invalidates retained measurement context for the addressed sensor. Use a typed
recorder method for every standard command that has one.

<!--! @section raw_extended_commands_pio PlatformIO Configuration -->

<!--! @include{lineno} extras/raw_extended_commands/platformio.ini -->

<!--! @section raw_extended_commands_code The Complete Example -->

<!--! @include{lineno} extras/raw_extended_commands/raw_extended_commands.ino -->
