# Non-Blocking Recorder API Test

This sketch compiles and exercises every public non-blocking recorder operation
and its result accessors. With one sensor at address `0`, the default path runs
address query, acknowledgement, identification, normal and concurrent
measurements, complete data retrieval, continuous data, and one raw command.

Address change, immediate data probing, and cancellation are disabled by
default because they can change or interrupt sensor state. Set their Boolean
test controls to `true` only when the attached hardware is prepared for those
effects.
