# Examples using the SDI-12 Library<!--! {#examples_page} -->

- Example A: Wild Card <!--! @subpage example_a_page -->
  - Gets sensor information from a single attached sensor and prints it to the serial port
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/a_wild_card)
- Example B: Address Change <!--! @subpage example_b_page -->
  - Allows you to change the address of your SDI-12 Sensor
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/b_address_change)
- Example C: Check All Addresses <!--! @subpage example_c_page -->
  - Checks all addresses for active sensors, and prints their status to the serial port
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/c_check_all_addresses)
- Example D: Simple Logger <!--! @subpage example_d_page -->
  - Checks all addresses for active sensors, and logs data for each sensor every minute
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/d_simple_logger)
- Example E: Continuous Measurement <!--! @subpage example_e_page -->
  - Checks all addresses for active sensors, and requests continuous data output from all of the sensors
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/e_continuous_measurement)
- Example F: Basic Data Request
  - This has been deprecated; use the Simple Logger or Continuous Measurement examples instead.
- Example G: SDI-12 Terminal and PC Interface <!--! @subpage example_g_page -->
  - Uses an Arduino as an interactive command terminal, passive bus monitor, or PC interface for SDI-12 sensors.
  - Replaces the deprecated Terminal Window and SDI-12 Interface examples.
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/g_terminal_interface)
- Example H: SDI-12 Slave Implementation <!--! @subpage example_h_page -->
  - Demonstrates using SDI-12 in slave mode
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/h_SDI-12_slave_implementation)
- Example I: SDI-12 Interface
  - This has been deprecated; use the combined SDI-12 Terminal and PC Interface example instead.
- Example J: External PCInt Library
  - This has been deprecated; the other examples now show how to handle interrupts for the SDI-12 library.
- Example K: Concurrent Logger <!--! @subpage example_k_page -->
  - Shows how to request concurrent measurements
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/k_concurrent_logger)
- Example L: Non-Blocking Concurrent Logger <!--! @subpage example_l_page -->
  - Interleaves concurrent measurements without waiting inside a recorder operation
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/l_non_blocking_concurrent_logger)
- Example M: Non-Blocking Measurement <!--! @subpage example_m_page -->
  - Demonstrates non-blocking acknowledgement, identification, CRC measurement,
    capacity-error handling, and complete data retrieval
  - [GitHub](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples/m_non_blocking_measurement)

## Advanced Examples

- [Raw Extended Commands](extras/raw_extended_commands/ReadMe.md)
  - Uses the recorder's secondary raw-command path for a manufacturer-specific
    extended command
