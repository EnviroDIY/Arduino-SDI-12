# Example H: Using SDI-12 in Slave Mode<!-- {#example_h_page} -->

Example sketch demonstrating how to implement an Arduino as a slave on an SDI-12 bus. This may be used, for example, as a middleman between an I2C sensor and an SDI-12 data logger.

Note that an SDI-12 slave must respond to M! or C! with the number of values it will report and the max time until these values will be available.  This example uses 9 values available in 21 s, but references to these numbers and the output array size and datatype should be changed for your specific application.

[//]: # ( @section h_SDI-12_slave_implementation_pio PlatformIO Configuration )

[//]: # ( @include{lineno} h_SDI-12_slave_implementation/platformio.ini )

[//]: # ( @section h_SDI-12_slave_implementation_code The Complete Example )
