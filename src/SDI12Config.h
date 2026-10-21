/**
 * @file SDI12Config.h
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief This file is used to configure the interrupt control and other library
 * settings/preferences for the SDI-12 Library.
 *
 * For the Arduino IDE, this is the only way to configure these settings. If you
 * are using PlatformIO, you have the option of using this file or changing your
 * configuration using build flags in your platformio.ini. Compile times will be
 * shorter if you use this file instead of build flags, but it is much easier to
 * track changes in your platformio.ini file than in this file.
 */


// Header Guards
#ifndef SRC_SDI12CONFIG_H_
#define SRC_SDI12CONFIG_H_

// Include Arduino.h to ensure variants file is pulled first
#include <Arduino.h>

//==============================================================
// Select the interrupt control method for AVR boards
// Uncomment the following line to use your own PCINT ISRs on AVR boards.
// This has no effect on non-AVR boards.

// #define SDI12_EXTERNAL_PCINT
//==============================================================

//==============================================================
// Select the interrupt vector for AVR boards
// To use a single interrupt vector, uncomment the appropriate line below.
// This only applies if you have **not** defined SDI12_EXTERNAL_PCINT.
// This has no effect on non-AVR boards.

// #define SDI12_PCINT0_ONLY
// #define SDI12_PCINT1_ONLY
// #define SDI12_PCINT2_ONLY
// #define SDI12_PCINT3_ONLY
//==============================================================

//==============================================================
// Response buffer size
//==============================================================
#if !defined(SDI12_BUFFER_SIZE) || defined(DOXYGEN)
/**
 * @brief The buffer size for incoming SDI-12 data.
 *
 * The default is 81 characters, which should be sufficient for most SDI-12 responses,
 * including high-volume ASCII responses:
 * - address is a single (1) character
 * - values take a maximum value of 35 characters for standard responses, or 75
 * characters for high-volume ASCII responses
 * - CRC is 3 characters
 * - CR is a single character
 * - LF is a single character
 *
 * If you need to use high volume **binary** responses, you may need to increase the
 * buffer size accordingly.
 * If your hard-pressed for memory and will not use high-volume ASCII responses, you can
 * drop this down to 41 characters.
 */
#define SDI12_BUFFER_SIZE 81
#endif

//==============================================================
// Define SDI12_TX_DISABLE_INTERRUPTS at build time to disable *all* interrupts while
// transmitting each character, on a processor where this library would otherwise leave
// them enabled.
//
// Processors running at 48MHz or faster are fast enough that a typical interrupt
// service routine cannot shift the bit timing past the SDI-12 tolerance, so since
// version 2.2.0 interrupts are left enabled while transmitting on those boards.  On a
// board running an RTOS - an ESP32 running FreeRTOS alongside WiFi or a cellular modem,
// for instance - a task switch or another driver's long ISR can still stretch a bit far
// enough to garble a command.  Defining this macro restores the pre-2.2.0 behavior of
// holding interrupts off for the time-critical part of every character.
//
// @warning On an ESP32 `noInterrupts()` masks interrupts for the entire core.  Each
// character takes ~8.33ms, so a short command holds them off for ~35ms.  Do not define
// this if something else on the board cannot tolerate that.

// #define SDI12_TX_DISABLE_INTERRUPTS
//==============================================================


//==============================================================
// Uncomment the line below to  override the #RX_WINDOW_FUDGE selected for your board.
// The value is in timer ticks, which is *not* the same unit on every board - see the
// per-board sections in SDI12_boards.h.  This is a tuning knob for an unusual bus (long
// cable, slow edges, a sensor with a sloppy baud rate); the default is right for a bus
// that meets the SDI-12 specification.

// Only change this if you know what you're doing!

// #define SDI12_RX_WINDOW_FUDGE 416
//==============================================================

//==============================================================
// Flexible SDI-12 Timing
//==============================================================
/**
 * @anchor sdi12_timing_defines
 * @name SDI-12 Timing Defines
 *
 * @brief Defines for SDI-12 protocol timing.
 *
 * @note Only change these if you know what you're doing!
 */
/**@{*/

#if !defined(SDI12_WAKE_DELAY) || defined(DOXYGEN)
/**
 * @def SDI12_WAKE_DELAY
 * @brief The default amount of additional time in milliseconds that the sensor takes to
 * wake before being ready to receive a command.  Default is 0ms - meaning the sensor is
 * ready for a command by the end of the 12ms break.  Per protocol, the wake time must
 * be less than 100 ms.
 */
#define SDI12_WAKE_DELAY 0
#endif

#if !defined(SDI12_LINE_BREAK_MICROS) || defined(DOXYGEN)
/**
 * @def SDI12_LINE_BREAK_MICROS
 * @brief The time in microseconds for the "break" before sending commands.  Per the
 * protocol, this must be >= 12ms.  The line level is HIGH for the break.
 */
#define SDI12_LINE_BREAK_MICROS static_cast<uint16_t>(12100)
#endif

#if !defined(SDI12_LINE_MARK_MICROS) || defined(DOXYGEN)
/**
 * @def SDI12_LINE_MARK_MICROS
 * @brief The time in microseconds for the required mark before a command or response.
 * Per the protocol, this must be >= 8.33ms.  The line level is LOW for the marking.
 */
#define SDI12_LINE_MARK_MICROS static_cast<uint16_t>(8400)
#endif
/**@}*/


#endif
