# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

An Arduino library (`SDI-12`, v2.3.3) that bit-bangs the SDI-12 sensor protocol (1200 baud, 7E1, inverted logic) on a single GPIO with no extra hardware. It descends from SoftwareSerial, with NeoSWSerial's timer-based receive so the processor is not stalled during Rx. Upstream is `EnviroDIY/Arduino-SDI-12`; this checkout's `origin` is the `similie` fork, which exists because ESP32 recorders reading older sensors regressed between v2.1.4 and v2.2.0+ (see the v2.3.3 entry in `ChangeLog.md` for the analysis). Fork releases are tagged `vX.Y.Z` and consumed by HyphenOS via `lib_deps = https://github.com/similie/Arduino-SDI-12.git#v2.3.3`. Version must agree in four places: `VERSION`, `library.properties`, `library.json`, and `PROJECT_NUMBER` in `docs/Doxyfile`.

## Commands

There is no root `platformio.ini`, no unit tests, and no host-side build — this is a library, and "the build" means compiling the example sketches against the boards CI targets. `pio` (PlatformIO Core) is installed locally; `arduino-cli`, `doxygen`, `clang-format`, and `cspell` are not.

```bash
# Compile one example for one board (this is exactly what CI does)
PLATFORMIO_CI_SRC=examples/d_simple_logger pio ci --board=uno --lib="."

# Several boards at once
PLATFORMIO_CI_SRC=examples/a_wild_card pio ci --board=mayfly --board=uno --board=featheresp32 --lib="."

# Override the pins an example uses (all examples guard these with #ifndef)
PLATFORMIO_BUILD_FLAGS="-D SDI12_DATA_PIN=5 -D SDI12_POWER_PIN=-1" PLATFORMIO_CI_SRC=examples/f_basic_data_request pio ci --board=uno --lib="."

# Example J is special: needs SDI12_EXTERNAL_PCINT and the EnableInterrupt library (AVR only)
PLATFORMIO_BUILD_FLAGS="-D SDI12_EXTERNAL_PCINT" PLATFORMIO_CI_SRC=examples/j_external_pcint_library \
  pio ci --board=uno --board=mayfly --lib="." --project-option="lib_deps=greygnome/EnableInterrupt@^1.1.0"

# Docs (needs doxygen 1.14; output lands in ../../Arduino-SDI-12_Doxygen, a sibling of the repo)
cd docs && doxygen Doxyfile

# Format (Google-based style, 88 cols, 2-space, aligned consecutive assignments/declarations)
clang-format -i src/*.cpp src/*.h examples/*/*.ino
```

CI boards (`.github/workflows/build_examples.yaml`): `mayfly envirodiy_stonefly_m4 uno megaatmega2560 leonardo zeroUSB arduino_nano_esp32 feather328p feather32u4 adafruit_feather_m0 adafruit_feather_m4 huzzah featheresp32 esp32-c3-devkitm-1 esp32-s3-devkitm-1`. Only `atmelavr` and `espressif32` platforms are installed locally; others download on first use. A change to the board-selection preprocessor in `SDI12_boards.h` should be compiled for at least one AVR, one SAMD21, one SAMD51, and one ESP32 target because each takes a different branch.

The per-example `platformio.ini` files are for users who installed the library from the registry (`src_dir` points into `.piolibdeps/`) — they are not usable for in-tree builds; use `pio ci` as above.

CI also runs `arduino-lint` in strict library-compliance mode (`verify_library_structure.yaml`) and a Doxygen build with `WARN_AS_ERROR = FAIL_ON_WARNINGS_PRINT` and `WARN_IF_UNDOCUMENTED = YES` — **an undocumented public member fails CI.** Commits containing `ci skip` bypass all three workflows. A bot nags PRs that don't touch `ChangeLog.md`; add entries under `## [Unreleased]`. Pushing a change to `VERSION` on `master` triggers the release workflow.

## Architecture

Two classes, four files in `src/`:

- **`SDI12` (`SDI12.h/.cpp`)** — a `Stream` subclass. Owns the protocol: state machine, Tx bit-banging, the Rx ISR, the receive ring buffer, CRC helpers, and SDI-12-specific `parseInt`/`parseFloat` overrides.
- **`SDI12Timer` (`SDI12_boards.h/.cpp`)** — the only board-specific layer. Picks a hardware timer/prescaler per MCU, saves and restores the timer registers, and converts tick deltas to bit counts.

### Board selection is entirely preprocessor

`SDI12_boards.h` has two `#if` ladders. The first keys on MCU/board macros and defines `TIMER_INT_SIZE` (8/16/32), `READTIME`, `TICKS_PER_SECOND`, and the timer to use; the second keys on `(TICKS_PER_SECOND, TIMER_INT_SIZE)` and derives `TICKS_PER_BIT`, `BITS_PER_TICK_Q10` (8-bit timers only), and `RX_WINDOW_FUDGE`. `SDI12_boards.cpp` has a matching ladder providing `configSDI12TimerPrescale()` / `resetSDI12TimerPrescale()` / `SDI12TimerRead()`. Unmatched boards hit `#error`. Adding a board means touching all three ladders.

| Target | Timer | Notes |
|---|---|---|
| ATmega 168/328/644/1284/1280/2560 | Timer2, 8-bit | conflicts with `tone()`; prescaler depends on `F_CPU` (16/12/8 MHz) |
| ATtiny 25/45/85 | Timer1, 8-bit | |
| ATmega32U4 (Leonardo, Feather 32u4) | Timer4 low 8 bits | |
| SAMD21 | GCLK4 → TC3, 16-bit | TC3 shares GCLK with TCC2 |
| SAMD51/SAME51 | GCLK6 → TC2, 16-bit | reading COUNT requires a READSYNC command first |
| ESP32/ESP8266, Particle, Giga, anything ≥ 48 MHz | `micros()`, 32-bit | no prescaler changes; `SDI12_YIELD_MS` defaults to 8 here |

Prescalers are changed in `begin()` and restored in `end()`, so `end()` matters on AVR.

### Receive path (see `docs/CreatingACharacter.md`)

A pin-change interrupt calls the static `SDI12::handleInterrupt()`, which forwards to `_activeObject->receiveISR()`. The ISR timestamps the edge, computes how many bit-times elapsed since the previous edge via `SDI12Timer::bitTimes()`, and fills that many bits of the character under construction (`rxState` / `rxMask` / `rxValue`). Completed characters are parity-checked and pushed to a **static** ring buffer `_rxBuffer[SDI12_BUFFER_SIZE]` (default 81, overridable). On 8-bit/8 MHz AVR (`TICKS_PER_SECOND == 31250`) the timer rolls over within one character, so the "ignore edges < 1 bit apart" guard is disabled there.

Parity is sticky: the first character that fails sets `_parityFailure`, and **every later character is dropped** until `setState(SDI12_TRANSMITTING)` (i.e. the next `sendCommand`/`sendResponse`) clears the flag. A parity error therefore truncates the whole response, not one character. `SDI12_IGNORE_PARITY` compiles the check out entirely.

Only one `SDI12` instance can receive at a time: `_activeObject` is a static pointer, and `setActive()` / `isActive()` switch it. Multiple bus objects share the one buffer.

### Transmit path

`writeChar()` busy-waits on `READTIME` for each bit. On boards below 48 MHz it runs with **all interrupts disabled** for most of the ~8.3 ms character. `wakeSensors()` drives a 12 ms break then 8.33 ms marking; `sendCommand()` = wake + write + `LISTENING`. `sendResponse()` (slave mode, example H) skips the break.

### State machine

`SDI12_STATES` = `DISABLED`, `ENABLED`, `HOLDING`, `TRANSMITTING`, `LISTENING`. `setState()` is the single place that sets `pinMode`, pin level, and whether the pin interrupt is enabled. `forceHold()` / `forceListen()` are the public entry points. The pin is always set `INPUT` + `LOW` first so the pull-up is off before switching to `OUTPUT`.

### Interrupt attachment — where the compatibility problems live

- **AVR default:** `SDI12.cpp` defines `ISR(PCINT0_vect)`…`ISR(PCINT3_vect)` itself and pokes `PCMSK`/`PCICR` directly (`setPinInterrupts()`). This is why the library cannot link alongside SoftwareSerial, NeoSWSerial, EnableInterrupt, Servo, etc. (see `docs/OverviewOfInterrupts.md`).
- **`SDI12_EXTERNAL_PCINT`:** compiles out the ISR vectors; the sketch must attach `SDI12::handleInterrupt` through another library (example J).
- **Non-AVR:** `attachInterrupt(digitalPinToInterrupt(pin), handleInterrupt, CHANGE)`; Particle lacks the macro and uses the raw pin. ESP ISRs carry `ISR_MEM_ACCESS` (= `IRAM_ATTR`).
- The `mayfly` branch narrows to PCINT3 only and `ExtInts` removes the vectors entirely; these are maintained as separate branches, not build flags.

### Stream overrides

`parseInt` / `parseFloat` / `peekNextDigit` are reimplemented for SDI-12 data responses (`+1.23-4.5` with sign as delimiter) and **ignore** their `LookaheadMode` / ignore-char arguments. The `LookaheadMode` enum is defined locally on cores that lack it (ESP8266, Particle, ESP32 core < 3.0.5).

### Compile-time knobs

`SDI12_BUFFER_SIZE`, `SDI12_WAKE_DELAY`, `SDI12_YIELD_MS` / `SDI12_YIELD()`, `SDI12_EXTERNAL_PCINT`, `SDI12_IGNORE_PARITY`, and — added by this fork in v2.3.3 — `SDI12_RX_WINDOW_FUDGE`, `SDI12_TX_DISABLE_INTERRUPTS`, `SDI12_LINE_BREAK_MICROS`, `SDI12_LINE_MARK_MICROS`. All are `#ifndef`-guarded so they can be set from build flags.

`RX_WINDOW_FUDGE` is the single most consequential number in the library: `bitTimes()` counts bits as `(dt + RX_WINDOW_FUDGE) / TICKS_PER_BIT`, so it is *exactly* how early an edge may arrive, and `TICKS_PER_BIT - RX_WINDOW_FUDGE` is how late. An asymmetric window is what broke the ESP32 path (50 µs early vs 783 late); the fork defaults the `micros()` path to half a bit (416). `extras/rx_window_model/rx_window_model.py` models both ISR generations and prints decode rates against edge skew, jitter, and baud error — use it before changing any timing constant.

## Examples and extras

Each `examples/<x>/` holds an `.ino`, a `ReadMe.md`, and a `platformio.ini`. Sketches begin with a Doxygen `@example{lineno}` block and read pins from `SDI12_DATA_PIN` / `SDI12_POWER_PIN`. A new example must be added to `examples/ReadMe.md` (with a `@subpage` tag) and to `examples_to_build` in `build_examples.yaml`; Doxygen picks it up automatically from `EXAMPLE_PATH`. `extras/` holds diagnostic sketches (bus spy, timing/warm-up testers, command tester) that CI does not build.

## Documentation conventions

Docs are Doxygen + m.css (`docs/mcss-Doxyfile`, `docs/mcss-conf.py`), published by `build_documentation.yaml` via the shared `EnviroDIY/workflows` actions. Markdown files are dual-rendered: `<!--! {#anchor} -->` and `<!--! @if GITHUB --> … <!--! @endif -->` blocks are invisible on GitHub but parsed by Doxygen, so keep both the GitHub TOC and the Doxygen `@tableofcontents` when editing `README.md` or `docs/*.md`. Doxygen preprocesses with `PREDEFINED = __AVR_ATmega1284P__ F_CPU=8000000L SDI12_EXTERNAL_PCINT ESP32 …`, so documentation comments must sit on the branches those macros select. `cspell.json` carries the project word list; add domain terms there rather than misspelling around them.
