[//]: # ( @page example_k_page Example K: Concurrent Measurements )
# Example K: Concurrent Measurements

This is very similar to example B - finding all attached sensors and logging data from them.
Unlike example B, however, which waits for each sensor to complete a measurement, this asks all sensors to take measurements concurrently and then waits until each is finished to query for results.
This can be much faster than waiting for each sensor when you have multiple sensor attached.

[//]: # ( @include{lineno} k_concurrent_logger.ino )
