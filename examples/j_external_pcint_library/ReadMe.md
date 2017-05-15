# Example J: Checks all addresses for active sensors, and logs data for each sensor every minute.

This is identical to example D, except that instead of using internal definitions of pin change interrupt vectors, it depends on another library to define them for it.

To use this example, you must remove the comment braces around "#define SDI12_EXTERNAL_PCINT" in the library and re-compile it.
