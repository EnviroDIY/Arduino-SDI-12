[//]: # ( @page example_j_page Example J: Using External Interrupts )
# Example J: Using External Interrupts

This is identical to example D, except that instead of using internal definitions of pin change interrupt vectors, it depends on another library to define them for it.

To use this example, you must remove the comment braces around `#define SDI12_EXTERNAL_PCINT` in the library and re-compile it.

[//]: # ( @section j_external_pcint_library_pio PlatformIO Configuration )

[//]: # ( @include{lineno} j_external_pcint_library/platformio.ini )

[//]: # ( @section j_external_pcint_library_code The Complete Example )
