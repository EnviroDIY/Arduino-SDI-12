[//]: # ( @page example_k_page Example K: Concurrent Measurements )
# Example K: Concurrent Measurements

This is very similar to example D - finding all attached sensors and logging data from them.
Unlike example D, however, which waits for each sensor to complete a measurement, this asks all sensors to take measurements concurrently and then waits until each is finished to query for results.
This can be much faster than waiting for each sensor when you have multiple sensor attached.

[//]: # ( @section k_concurrent_logger_pio PlatformIO Configuration )

[//]: # ( @include{lineno} k_concurrent_logger/platformio.ini )

[//]: # ( @section k_concurrent_logger_code The Complete Example )
