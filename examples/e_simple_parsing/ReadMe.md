[//]: # ( @page example_e_page Example E: Parsing Integers and Floats from the Buffer )
# Example E: Parsing Integers and Floats from the Buffer

This example demonstrates the ability to parse integers and floats from the buffer.
It is based closely on example D, however, every other time it prints out data, it multiplies the data by a factor of 2.
The resulting table will be something like this:

|Time Elapsed (s)| Sensor Address and ID| Measurement 1   | Measurement 2     | ... etc.|
|----------------|----------------------|-----------------|-------------------|---------|
| 6              | c13SENSOR ATM   311  | 0.62 x 2 = 1.24 | 19.80 x 2 = 39.60 |         |
| 17             | c13SENSOR ATM   311  | 0.62            | 19.7              |         |
| 29             | c13SENSOR ATM   311  | 0.62 x 2 = 1.24 | 19.70 x 2 = 39.40 |         |
| 41             | c13SENSOR ATM   311  | 0.62            |19.8               |         |

This is a trivial and nonsensical example, but it does demonstrate the ability to manipulate incoming data.

[//]: # ( @section e_simple_parsing_pio PlatformIO Configuration )

[//]: # ( @include{lineno} e_simple_parsing/platformio.ini )

[//]: # ( @section e_simple_parsing_code The Complete Example )
