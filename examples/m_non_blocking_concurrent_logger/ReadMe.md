# Example M: Non-Blocking Concurrent Logger<!--! {#example_m_page} -->

This advanced example has functionality similar to Example K, but it never
waits inside a recorder transaction. `loop()` repeatedly calls
`SDI12Recorder::update()` while a small application state machine scans the bus,
starts concurrent measurements, and retrieves each sensor's complete data when
`isMeasurementReady()` reports readiness.

The logger starts as many measurements as the recorder's fixed context table can
retain. A `RESULT_CONTEXT_CAPACITY` result ends the start pass without changing
the library's default configuration.

<!--! @section m_non_blocking_concurrent_logger_pio PlatformIO Configuration -->

<!--! @include{lineno} m_non_blocking_concurrent_logger/platformio.ini -->

<!--! @section m_non_blocking_concurrent_logger_code The Complete Example -->

<!--! @include{lineno} m_non_blocking_concurrent_logger/m_non_blocking_concurrent_logger.ino -->
