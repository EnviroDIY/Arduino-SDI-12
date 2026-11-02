# SDI-12 Recorder API<!--! {#recorder_api_page} -->

`SDI12Recorder` is the high-level interface for an Arduino acting as an SDI-12
data recorder. It constructs standard commands, validates complete responses,
applies the retry sequence from Appendix B of the SDI-12 specification, and
tracks pending measurements without dynamic allocation.

## Choosing an API Style<!--! {#recorder_api_styles} -->

Use the blocking methods for simple sketches:

- `queryAddress()` discovers the only sensor on a bus.
- `requestAcknowledgement()`, `identify()`, and `changeAddress()` perform the
  corresponding single command.
- `measure()` performs a normal `M` or CRC-requesting `MC` measurement and
  retrieves every advertised value from `D0` through the necessary final data
  block.
- `requestContinuousData()` performs one independent `R0` through `R9`, or
  CRC-requesting `RC0` through `RC9`, request.

Use the non-blocking methods when the sketch must continue other work while an
SDI-12 transaction is waiting. Start one operation, call `update()` frequently,
and wait for `isOperationComplete()` before inspecting `result()` or starting
the next bus operation. A caller-owned data buffer passed to `requestData()` or
`startRequestContinuousData()` must remain valid and unchanged until the
operation completes or is cancelled.

The [non-blocking measurement example](@ref example_m_page) shows the complete
acknowledgement, identification, measurement, and data sequence. The
[non-blocking concurrent logger](@ref example_l_page) shows how one executor
interleaves operations for several sensors.

## Measurements and Data Context<!--! {#recorder_measurement_context} -->

A successful normal or concurrent measurement retains the sensor address, CRC
policy, expected value count, and readiness state. `requestData()` uses that
context to generate and validate the required `D0` through `D9` commands. It
releases the context only after complete retrieval or a sensor-reported aborted
measurement.

The recorder has `SDI12_RECORDER_CONTEXT_SLOTS` fixed context slots. The
default is three. Starting another measurement when every slot is occupied
returns `RESULT_CONTEXT_CAPACITY` before transmitting. Increasing the setting
uses more RAM in every recorder object, so beginner examples leave the default
unchanged.

If the caller's data buffer is smaller than the advertised value count,
`requestData()` returns `RESULT_DATA_BUFFER_TOO_SMALL` without transmitting and
leaves the measurement context available. The caller can then retry with a
larger buffer.

## Results and Responses<!--! {#recorder_results} -->

Every operation publishes a `RecorderResult`. `RESULT_SUCCESS` is the only
successful terminal result. Use `printRecorderResult()` for a readable
diagnostic. Data operations also publish `dataValueCount()`, while address,
identification, and measurement operations provide typed accessors documented
with the `SDI12Recorder` class.

The response payload returned by `response()` is recorder-owned. Use
`copyResponse()` when the application must retain it after another operation.

## Raw Extended Commands<!--! {#recorder_raw_commands} -->

`startRawCommand()` is a secondary interface for manufacturer-specific commands
that have no typed recorder method. Supply the sensor address and only the
command body; the recorder adds the address and terminating exclamation point.
It validates the returned address and retry timing but treats the remaining
payload as opaque. The caller is responsible for command-specific parsing and
validation.

A raw command invalidates retained measurement context for its addressed
sensor. Prefer typed methods for standard commands because they provide strict
grammar and CRC validation and manage multi-command workflows automatically.

See the [raw extended-command example](@ref raw_extended_commands_page) for a
minimal polling workflow.

## Configuration<!--! {#recorder_configuration} -->

All user-selectable settings are in `SDI12Config.h`:

- `SDI12_RECORDER_CONTEXT_SLOTS` controls retained measurement capacity.
- `SDI12_RECORDER_RESPONSE_BUFFER_SIZE` controls recorder-owned ASCII response
  storage.
- `SDI12_RECORDER_ALWAYS_BREAK` disables the optimized legal no-break path for
  the first attempt of a new operation.

These are compile-time settings. Change them only when the application has a
specific capacity or compatibility requirement.
