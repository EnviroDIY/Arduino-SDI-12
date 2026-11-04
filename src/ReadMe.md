# Library Source Files

The `src` folder contains the public library headers and their implementations:

- `SDI12.h` and `SDI12.cpp` provide the low-level, stream-compatible SDI-12 transport.
- `SDI12Protocol.h` and `SDI12Protocol.cpp` provide shared protocol constants, address conversion, CRC helpers, and timing limits.
- `SDI12Recorder.h` and `SDI12Recorder.cpp` provide the high-level recorder API for validated commands, retries, measurement tracking, and complete data retrieval.
- `SDI12Config.h` contains all user-selectable library configuration macros.
- `SDI12_boards.h` and `SDI12_boards.cpp` contain board- and timer-specific support.

Recorder applications should normally include `SDI12.h` and `SDI12Recorder.h`, construct one `SDI12Recorder` from an `SDI12` transport, and use the recorder's typed command methods.
The blocking methods are intended for beginner sketches.
The non-blocking methods start an operation and depend on frequent calls to `SDI12Recorder::update()` until `SDI12Recorder::isOperationComplete()` is true.

See the [recorder API guide](../docs/SDI12Recorder.md) for blocking and non-blocking workflows, measurement-context behavior, result handling, and raw extended commands.
