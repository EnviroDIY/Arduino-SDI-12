# Example M: Non-Blocking Measurement<!--! {#example_m_page} -->

This advanced example uses the polling API for acknowledgement, identification,
a CRC-requesting normal measurement, and complete data retrieval. It also makes
one intentionally undersized data request to show that the recorder reports
`RESULT_DATA_BUFFER_TOO_SMALL` without transmitting or discarding the retained
measurement context.

The caller-owned data array remains in scope for the entire asynchronous data
operation, and `SDI12Recorder::update()` is called on every pass through
`loop()`.

<!--! @section m_non_blocking_measurement_pio PlatformIO Configuration -->

<!--! @include{lineno} m_non_blocking_measurement/platformio.ini -->

<!--! @section m_non_blocking_measurement_code The Complete Example -->

<!--! @include{lineno} m_non_blocking_measurement/m_non_blocking_measurement.ino -->
