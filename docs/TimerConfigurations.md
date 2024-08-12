# Timer Configurations

[//]: # ( @tableofcontents )

[//]: # ( @cond GitHub )

- [Timer Configurations](#timer-configurations)
  - [SDI-12 Timing Rules](#sdi-12-timing-rules)
  - [Ideal Timer Settings](#ideal-timer-settings)
  - [AVR Boards](#avr-boards)
    - [Available Timers on AVR Boards](#available-timers-on-avr-boards)
      - [ATmega AVR Available Timers](#atmega-avr-available-timers)
      - [ATtiny AVR Available Timers](#attiny-avr-available-timers)
      - [ATmegaXU AVR Available Timers](#atmegaxu-avr-available-timers)
    - [Timers Used by Arduino AVR Core](#timers-used-by-arduino-avr-core)
    - [Selected AVR Timers for SDI-12](#selected-avr-timers-for-sdi-12)
      - [ATmega AVR Selected Timers](#atmega-avr-selected-timers)
      - [ATtiny AVR Selected Timers](#attiny-avr-selected-timers)
      - [ATmegaXU Selected Timers](#atmegaxu-selected-timers)
  - [SAMD Boards](#samd-boards)
    - [SAMD21](#samd21)
      - [Available Clocks and Timers on SAMD21 Boards](#available-clocks-and-timers-on-samd21-boards)
        - [SAMD21 Generic Clock Generators](#samd21-generic-clock-generators)
        - [SAMD21 Timer Controllers](#samd21-timer-controllers)
      - [Timers Used by Arduino SAMD21 Core](#timers-used-by-arduino-samd21-core)
      - [Selected SAMD21 Timers for SDI-12](#selected-samd21-timers-for-sdi-12)
    - [SAMD51/SAME51](#samd51same51)
      - [Available Clocks and Timers on SAMD51 Boards](#available-clocks-and-timers-on-samd51-boards)
        - [SAMD51 Generic Clock Generators](#samd51-generic-clock-generators)
        - [SAMD51 Timer Controllers](#samd51-timer-controllers)
      - [Timers Used by Arduino SAMD51 Core](#timers-used-by-arduino-samd51-core)
      - [Selected SAMD51 Timers for SDI-12](#selected-samd51-timers-for-sdi-12)
  - [Other Boards](#other-boards)


[//]: # ( @endcond )

This library listens for pin level changes and then use a timer to calculate how many databits have been sent since the last change and to convert that to a character.
The speed of the timer is dependent on the speed of the processor and "dividers" and "prescalers" used to slow the effective clock.
Unfortunately, the "ticks" of the processor clock aren't perfectly aligned with the times of the level changes from the SDI-12 device.
With the clocks not perfectly aligned, we can't know exactly the time that a bit started or ended, just the time of the last readable tick.
This means we need to do some averaging and "fudging" to align the two.

@see rx_page for more information on how a character is created.

## SDI-12 Timing Rules

SDI-12 Communicates at 1200 baud (bits/s) and sends each character using 10 bits (7E1).

- Character Times:
  - 1 bit = 0.83333 ms (?? tolerance ??)
  - 1 character = 8.33333 ms
  - maximum marking between character stop (LOW) and next character start (HIGH) = 1.66ms (**no tolerance**)
    - This is equivalent to 2 bits!

- Break and Marking Times
  - maximum sensor wake time (between a break and start bit) = 100ms ± 0.4ms
  - minimum marking (LOW) return-to-sleep time = 100ms ± 0.4ms
  - recorder break (HIGH) between commands =  >12ms ± 0.4ms (ie, >12.5ms)
  - maximum recorder marking (LOW) before a start bit (HIGH) = >8.33ms ± 0.4ms (ie, >8.73ms)
  - maximum time before relinquishing line control after stop bit = 7.5ms ± 0.4ms
  - maximum marking (LOW) before a new waking break (HIGH) must be issued = 87ms ± 0.4ms
    - A break is also required when switching between sensors

- Retry Times
  - There are two retry "loops" - an "inner" loop of retries without breaks between and an "outer" loop of retries with breaks in between
  - Inner Retries (*without* breaks between)
    - Response window before a retry = 16.67ms - 87ms (< 87ms = time before a break is required)
    - A minimum of 3 "inner" retries are required.
    - At least one of the "inner" retries must start >100ms after the falling edge (end) of the break that started the innter retry loop.
  - Outer Retries (*with* breaks between)
    - Outer retries are used after >112.5ms of inner retries have been attempted
    - A minimum of 3 "outer" retries are required.

## Ideal Timer Settings

When setting up our timers, the goal is to be able to have as many ticks as possible for each bit.
The more ticks we have, the better job we can do with the needed averaging and fudging.
With <10 ticks/bit, we probably won't be accurate enough to be functional.
But, we also need to make sure that the clock timer doesn't roll over before the end of the 8.33ms required for each character.
When acting as a recording device, it would be even better if the timer could last until the end of a 112.5ms retry timer before rolling over.
There is no benefit to the timer lasting longer than 112.5ms before rolling over.

Each timer has finite options for pre-scaling, often in powers of 2.
To catch all the bits we need, when selecting the prescaler, we must round **UP** to the next closest available prescaler number (round **DOWN** the Hz) to give us *more* ticks than required.

Using a 16 bit counter, the counter rolls after 65536 ticks.

- Going for the maximum retry time, 112.5ms / 65536 ticks
  - 1.71661376953125 µsec/tick, 582.54222 kHz
  - 485.25767 ticks/bit
  - This is *plenty* of ticks per bit!  With a 16-bit timer, there's no reason to not use this whole period.
- Going for the minimum 8.33ms per character, 8.33ms / 65536 ticks
  - 0.127105712890625 µsec/tick, 7.86747 MHz
  - 6553.6 ticks/bit
  - This is overkill.

- Conclusion: With a 16-bit timer, select the smallest prescaler possible that keeps the speed *below* 582 kHz

If we only have an 8 bit timer, the counter rolls after 256 ticks.

- Going for the maximum retry time, 112.5ms / 256 ticks
  - 0.439453125 msec/tick, 2.275 kHz
  - 1.89554 ticks/bit
  - This is no where near enough bits / tick for accuracy, so it is not possible to keep a timer running for the 112.5 ms retry with a 8-bit timer.
- Going for the minimum 8.33ms per character, 8.33ms / 256 ticks = 25.6 ticks/bit
  - 0.0325390625 msec/tick, 30.73229 kHz
  - 25.6 ticks/bit

- Conclusion: With a 8-bit timer, select the smallest prescaler possible that keeps the speed *below* 30 kHz

## AVR Boards

### Available Timers on AVR Boards

#### ATmega AVR Available Timers

[ATmega164A/PA/324A/PA/644A/PA/1284/P](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-8272-8-bit-AVR-microcontroller-ATmega164A_PA-324A_PA-644A_PA-1284_P_datasheet.pdf)

>
> - Up to 20MIPS throughput at 20MHz
>   - Most Arduino boards are run at 16 or 8 MHz with a few at 12 MHz
> - Two 8-bit Timer/Counters with Separate Prescalers and Compare Modes
>   - Timers 0 and 2
>   - Prescalers available at 8/64/256/1024 on Timer 0
>   - Prescalers available at 8/32/64/128/256/1024 on Timer 2
> - One/two 16-bit Timer/Counter with Separate Prescaler, Compare Mode, and Capture Mode
>   - Timers 1 and 3
>   - Prescalers available at 8/64/256/1024 on Timers 1 and 3
>   - Timer 3 is only available on the 1284p

[ATmega640/V-1280/V-1281/V-2560/V-2561/V](https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf)

>
> - Up to 16 MIPS Throughput at 16MHz
> – Two 8-bit Timer/Counters with Separate Prescaler and Compare Mode
>   - Timers 0 and 2
>   - Prescalers available at 8/64/256/1024 on Timer 0
>   - Prescalers available at 8/32/64/128/256/1024 on Timer 2
> – Four 16-bit Timer/Counters with Separate Prescaler, Compare- and Capture Mode
>   - Timers 1, 3, 4, and 5
>   - Prescalers available at 8/64/256/1024 on Timer 1, 3, 4, and 5

#### ATtiny AVR Available Timers

[ATtiny25/V / ATtiny45/V / ATtiny85/V](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf)

>
> - Up to 20MIPS throughput at 20MHz
>   - Most Arduino boards are run at 16 or 8 MHz with a few at 12 MHz
> – One 8-bit Timer/Counter with Prescaler and Two PWM Channels
>   - Timer 0
>   - Prescalers available at 8/64/256/1024
> – One 8-bit High Speed Timer/Counter with Separate Prescaler
>   - Timer 1
>   - Prescalers available at 64/128/256/512/1024/2048/4096/8192/16384

#### ATmegaXU AVR Available Timers

[ATmega16U4/ATmega32U4](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf)

>
> – Up to 16 MIPS Throughput at 16MHz
> – One 8-bit Timer/Counter with Separate Prescaler and Compare Mode
>   - Timer 0
>   - Prescalers available at 8/64/256/1024 on Timer 0
> – Two 16-bit Timer/Counter with Separate Prescaler, Compare- and Capture Mode
>   - Timers 1 and 3
>   - Prescalers available at 8/64/256/1024 on Timer 1 and 3
> – One 10-bit High-Speed Timer/Counter with PLL (64MHz) and Compare Mode
>   - Timer 4
>   - Prescalers available at 2/4/8/16/32/64/128/256/512/1024/2048/8192/169384 on Timer 4

> [!NOTE]
> There is no Timer 2 on the 16U4 or the 32U4

### Timers Used by Arduino AVR Core

- Timer 0
  - [The primary clock (millis)](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/wiring.c)
  - Fast hardware PWM
- Timer 1
  - Phase-correct hardware PWM
  - [AltSoftSerial](https://github.com/PaulStoffregen/AltSoftSerial/)
  - Primary for [Servo](https://github.com/arduino-libraries/Servo)
- Timer 2
  - Primary for [Tone](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Tone.cpp) (except on the 16U4/32U4)
  - Phase-correct hardware PWM
  - [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial/)
- Timer 3
  - Primary for [Tone](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Tone.cpp) (only on the 16U4/32U4)
  - Optional for [Tone](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Tone.cpp) on some boards
  - Optional for [Servo](https://github.com/arduino-libraries/Servo) on some boards
- Timer 4
  - Optional for [Tone](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Tone.cpp) on some boards
  - Optional for [Servo](https://github.com/arduino-libraries/Servo) on some boards
- Timer 5
  - Optional for [Tone](https://github.com/arduino/ArduinoCore-avr/blob/master/cores/arduino/Tone.cpp) on some boards
  - Optional for [Servo](https://github.com/arduino-libraries/Servo) on some boards

### Selected AVR Timers for SDI-12

#### ATmega AVR Selected Timers

For simplicity, we use Timer/Counter 2 for both ATmega164A/PA/324A/PA/644A/PA/1284/P and ATmega640/V-1280/V-1281/V-2560/V-2561/V series boards.

>
> Timer/Counter2 (TC2) is a general purpose, single channel, 8-bit Timer/Counter module.
>
> **Features of Timer/Counter 2**
>
> - Single Channel Counter
> - Clear Timer on Compare Match (Auto Reload)
> - Glitch-free, Phase Correct Pulse Width Modulator (PWM)
> - Frequency Generator
> - 10-bit Clock Prescaler
> - Overflow and Compare Match Interrupt Sources (TOV2, OCF2A, and OCF2B)
> - Allows Clocking from External 32kHz Watch Crystal Independent of the I/O Clock

#### ATtiny AVR Selected Timers

On the ATTiny series (ATtiny25/V / ATtiny45/V / ATtiny85/V) boards, we use Timer/Counter 1

>
> The Timer/Counter1 features a high resolution and a high accuracy usage with the lower prescaling opportunities.
> It can also support two accurate, high speed, 8-bit pulse width modulators using clock speeds up to 64MHz (or 32MHz in low speedmode).

#### ATmegaXU Selected Timers

On the AtMega16U4 and AtMega32U4, we use Timer/Counter 4 as an 8-bit timer.

>
> Timer/Counter4 is a general purpose high speed Timer/Counter module, with three independent Output Compare Units, and with enhanced PWM support.
>
> **Features of Timer/Counter 4**
>
> - Up to 10-Bit Accuracy
> - Three Independent Output Compare Units
> - Clear Timer on Compare Match (Auto Reload)
> - Glitch Free, Phase and Frequency Correct Pulse Width Modulator (PWM)
> - Enhanced PWM mode: one optional additional accuracy bit without effect on output frequency
> - Variable PWM Period
> - Independent Dead Time Generators for each PWM channels
> - Synchronous update of PWM registers
> - Five Independent Interrupt Sources (TOV4, OCF4A, OCF4B, OCF4D, FPF4)
> - High Speed Asynchronous and Synchronous Clocking Modes
> - Separate Prescaler Unit

> [!NOTE]
> We only utilize the low byte register of Timer 4, effectively using the 10-bit timer as an 8-bit timer.

## SAMD Boards

### SAMD21

#### Available Clocks and Timers on SAMD21 Boards

##### SAMD21 Generic Clock Generators

>
> The Generic Clock controller GCLK provides nine Generic Clock Generators that can provide a wide range of clock frequencies.
> Generators can be set to use different external and internal oscillators as source.
> The clock of each Generator can be divided.
> The outputs from the Generators are used as sources for the Generic Clock Multiplexers, which provide the Generic Clock (GCLK_PERIPHERAL) to the peripheral modules, as shown in Generic Clock Controller Block Diagram.
>
> **Features of the Generic Clock Generator**
>
> - Provides Generic Clocks
> - Wide frequency range
> - Clock source for the generator can be changed on the fly

##### SAMD21 Timer Controllers

>
> The TC consists of a counter, a prescaler, compare/capture channels and control logic.
> The counter can be set to count events, or it can be configured to count clock pulses.
> The counter, together with the compare/capture channels, can be configured to timestamp input events, allowing capture of frequency and pulse width.
> It can also perform waveform generation, such as frequency generation and pulse-width modulation (PWM).
>
> **Features of the Timer Controller**
>
> - Selectable configuration
>   – Up to five 16-bit Timer/Counters (TC) including one low-power TC, each configurable as:
>     - 8-bit TC with two compare/capture channels
>     - 16-bit TC with two compare/capture channels
>     - 32-bit TC with two compare/capture channels, by using two TCs
> - Waveform generation
>     – Frequency generation
>     – Single-slope pulse-width modulation
> - Input capture
>     – Event capture
>     – Frequency capture
>     – Pulse-width capture
> - One input event
> - Interrupts/output events on:
>     – Counter overflow/underflow
>     – Compare match or capture
> - Internal prescaler
> - Can be used with DMA and to trigger DMA transactions

#### Timers Used by Arduino SAMD21 Core

The Adafruit Arduino core uses:

- 0 as GENERIC_CLOCK_GENERATOR_MAIN (the main clock)


The Adafruit Arduino core uses:

- TC5 for Tone
- TC4 for Servo

#### Selected SAMD21 Timers for SDI-12

For SDI-12, we'll use Generic Clock Generator 4 and Timer Controller 3

### SAMD51/SAME51

#### Available Clocks and Timers on SAMD51 Boards

##### SAMD51 Generic Clock Generators

>
> Depending on the application, peripherals may require specific clock frequencies to operate correctly.
> The Generic Clock controller (GCLK) features 12 Generic Clock Generators [11:0] that can provide a wide range of clock frequencies.
>
> Generators can be set to use different external and internal oscillators as source.
> The clock of each Generator can be divided.
> The outputs from the Generators are used as sources for the Peripheral Channels, which provide the Generic Clock (GCLK_PERIPH) to the peripheral modules, as shown in Figure 14-2.
> The number of Peripheral Clocks depends on how many peripherals the device has.
>
> NOTE: The Generator 0 is always the direct source of the GCLK_MAIN signal.
>
> **Features of the Generic Clock Generators**
>
> - Provides a device-defined, configurable number of Peripheral Channel clocks
> - Wide frequency range
>   - Various clock sources
>   - Embedded dividers

##### SAMD51 Timer Controllers

>
> There are up to eight TC peripheral instances.
>
> Each TC consists of a counter, a prescaler, compare/capture channels and control logic.
> The counter can be set to count events, or clock pulses.
> The counter, together with the compare/capture channels, can be configured to timestamp input events or IO pin edges, allowing for capturing of frequency and/or pulse width.
>
> A TC can also perform waveform generation, such as frequency generation and pulse-width modulation.
>
> **Features of the Timer Controllers**
>
> - Selectable configuration
>   - 8-, 16- or 32-bit TC operation, with compare/capture channels
> - 2 compare/capture channels (CC) with:
>   - Double buffered timer period setting (in 8-bit mode only)
>   - Double buffered compare channel
> - Waveform generation
>   - Frequency generation
>   - Single-slope pulse-width modulation
> - Input capture
>   - Event / IO pin edge capture
>   - Frequency capture
>   - Pulse-width capture
>   - Time-stamp capture
>   - Minimum and maximum capture
> - One input event
> - Interrupts/output events on:
>   - Counter overflow/underflow
>   - Compare match or capture
> - Internal prescaler
> - DMA support

#### Timers Used by Arduino SAMD51 Core

The Adafruit Arduino core uses:

- 0 as GENERIC_CLOCK_GENERATOR_MAIN (the main clock, sourced from MAIN_CLOCK_SOURCE = GCLK_GENCTRL_SRC_DPLL0)
- 1 as GENERIC_CLOCK_GENERATOR_48M (48MHz clock for USB and 'stuff', sourced from GCLK_GENCTRL_SRC_DPLL0)
- 2 as GENERIC_CLOCK_GENERATOR_100M (100MHz clock for other peripherals, sourced from GCLK_GENCTRL_SRC_DPLL1)
- 3 as GENERIC_CLOCK_GENERATOR_XOSC32K (32kHz oscillator, sourced from 32kHz external oscillator GCLK_GENCTRL_SRC_XOSC32K)
- 4 as GENERIC_CLOCK_GENERATOR_12M (12MHz clock for DAC, sourced from GCLK_GENCTRL_SRC_DPLL0)
- 5 as GENERIC_CLOCK_GENERATOR_1M (??, sourced from  CLK_GENCTRL_SRC_DPLL0)


The Adafruit Arduino core uses:

- TC0 for Tone (though any other timer may be used, if another pin is selected)
- TC1 for Servo (though any other timer may be used, if another pin is selected)

#### Selected SAMD51 Timers for SDI-12

For SDI-12, we'll use Generic Clock Generator 6 and Timer Controller 2

## Other Boards

For sufficiently fast boards, instead of using a dedicated processor timer, we can use the built-in `micros()` function as the timer.

From calculations using https://github.com/SRGDamia1/avrcycle, the micros() function takes about 60 (!!) clock cycles on a Mayfly.
We're going to blindly assume that the micros() function takes up about the same number of clock cycles for all Arduino boards.
This is probably a huge assumption, but go with it.
If we're going to use micros() fortiming, lets set a minimum usable CPU speed of the micros() function being accurate to 1µs.
That means we need to get 60 ticks/1µs or 60MHz.
Ehh.. Maybe we'll be generous and allow it down to 48MHz in the code.
That will allow Rensas AVR processors to attempt SDI-12.

I know from testing, that we *cannot* use micros on a board 8MHz AVR board, but that it does work on a 80MHz Espressif8266.

> [!WARNING]
> I haven't actually tested the minimum speed that this will work at!

@todo: Test 48MHz

Both the ESP8266 and ESP32 are definitely fast enough that this works.

- The ESP8266 runs at either 80 or 160 MHz
- The ESP32 runs at 160 or 240 MHz.

All of the other processors using the Arduino core also have the micros function, but the rest are not fast enough to waste the processor cycles to use the micros function and must manually configure the processor timer and use the faster assembly macros to read that processor timer directly.
