/**
 * @file SDI12_boards.h
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano (sdamiano@stroudcenter.org)
 *
 * @brief This file defines the timing units needed for various Arduino boards.
 *
 */

/* ======================== Arduino SDI-12 =================================
An Arduino library for SDI-12 communication with a wide variety of environmental
sensors. This library provides a general software solution, without requiring
   ======================== Arduino SDI-12 =================================*/

#ifndef SRC_SDI12_BOARDS_H_
#define SRC_SDI12_BOARDS_H_

#include <Arduino.h>
/**
 * @def ISR_MEM_ACCESS
 * @brief Defines a memory access location, if needed for the interrupts service
 * routines.
 *
 * On espressif boards (ESP8266 and ESP32), the ISR must be stored in IRAM
 */
#if defined(ESP32) || defined(ESP8266)
#define ISR_MEM_ACCESS IRAM_ATTR
#else
#define ISR_MEM_ACCESS
#endif  // defined(ESP32) || defined(ESP8266)


/**
 * @def TIMER_IN_USE_STR
 * @brief A string description of the timer to use
 *
 * @def TIMER_INT_TYPE
 * @brief The interger type of the timer.
 *
 * @def TIMER_INT_SIZE
 * @brief The size in bits of the timer count value.
 *
 * @def READTIME
 * @brief The function or macro used to read the clock timer value.
 *
 * @def PRESCALE_IN_USE_STR
 * @brief A string description of the prescaler in use.
 *
 * @def TICKS_PER_SECOND
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * @def TICKS_PER_BIT
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
 * @def BITS_PER_TICK_Q10
 * @brief The number of "ticks" of the timer per SDI-12 bit, shifted by 2^10.
 *
 * @def RX_WINDOW_FUDGE
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */


// Most 'standard' AVR boards
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || \
  defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) ||  \
  defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) ||   \
  defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)

// Use Timer/Counter 2 on most AVR boards
#define TIMER_IN_USE_STR "Timer2"
// Timer 2 on AtMega boards is an 8-bit timer
#define TIMER_INT_TYPE uint8_t
#define TIMER_INT_SIZE 8
#define READTIME TCNT2

#if F_CPU == 16000000L
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
// 16MHz / 1024 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
#define TICKS_PER_SECOND 15625

#elif F_CPU == 12000000L
#define PRESCALE_IN_USE_STR "12MHz/1024=11.7kHz"
// 12MHz / 1024 prescaler = 11719 'ticks'/sec = 85.33 µs / 'tick'
#define TICKS_PER_SECOND 11719

#elif F_CPU == 8000000L
#define PRESCALE_IN_USE_STR "8MHz/256=31.25kHz"
// 8MHz / 256 prescaler = 31250 'ticks'/sec = 32 µs / 'tick'
#define TICKS_PER_SECOND 31250

#endif  // F_CPU


// ATtiny boards (ie, adafruit trinket)
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

// On the ATTiny boards, we use Timer/Counter 1
#define TIMER_IN_USE_STR "Timer1"
// Timer 1 on the ATTiny boards is an 8-bit timer
#define TIMER_INT_TYPE uint8_t
#define TIMER_INT_SIZE 8
#define READTIME TCNT1

#if F_CPU == 16000000L
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
// 16MHz / 1024 prescaler = 15625 'ticks'/sec = 15.625 kHz = 64 µs / 'tick'
#define TICKS_PER_SECOND 15625

#elif F_CPU == 8000000L
#define PRESCALE_IN_USE_STR "8MHz/512=15.625kHz"
// 8MHz / 512 prescaler = 15625 'ticks'/sec = 15.625 kHz = 64 µs / 'tick'
#define TICKS_PER_SECOND 15625

#endif  // F_CPU


// Arduino Leonardo & Yun and other 32U4 boards
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || \
  defined(__AVR_ATmega32U4__)

// On the AtMega16U4 and AtMega32U4, we use Timer/Counter 4 as an 8-bit timer.
#define TIMER_IN_USE_STR "Timer4"
/**
 * Timer 4 on the U4 series is an 10-bit timer, but we're only using the lower 8 bits
 */
#define TIMER_INT_TYPE uint8_t
#define TIMER_INT_SIZE 8
#define READTIME TCNT4

#if F_CPU == 16000000L
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
// 16MHz / 1024 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
#define TICKS_PER_SECOND 15625

#elif F_CPU == 8000000L
#define PRESCALE_IN_USE_STR "8MHz/512=15.625kHz"
// 8MHz / 512 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
#define TICKS_PER_SECOND 15625

#endif  // F_CPU


// Arduino Zero other SAMD21 boards
#elif defined(ARDUINO_SAMD_ZERO) || defined(__SAMD21G18A__) || \
  defined(__SAMD21J18A__) || defined(__SAMD21E18A__)

// For SDI-12, we'll use Generic Clock Generator 4 and Timer Controller 3
#define TIMER_IN_USE_STR "GCLK4-TC3"
// We're using the timer in 16-bit mode
#define TIMER_INT_TYPE uint16_t
#define TIMER_INT_SIZE 16

/// The timer controller to use
#define SDI12_TC TC3

// This signifies the register of timer/counter 3, the 16-bit count, the count value
// This is equivalent to TC3->COUNT16.COUNT.reg
#define READTIME REG_TC3_COUNT16_COUNT

#define PRESCALE_IN_USE_STR "48MHz/6/16=500kHz"
// Start with 48MHz "main" clock source (GCLK_GENCTRL_SRC_DFLL48M)
// 48MHz / 6x clock source divider (GCLK_GENDIV_DIV(6)) = 8MHz
// 8MHz / 16x prescaler (TC_CTRLA_PRESCALER_DIV16) =  500kHz = 500,000 'ticks'/sec
#define TICKS_PER_SECOND 500000


// SAMD51 and SAME51 boards
#elif defined(__SAMD51__) || defined(__SAME51__)

// For SDI-12, we'll use Generic Clock Generator 6 and Timer Controller 2
#define TIMER_IN_USE_STR "GCLK6-TC2"
//  We're using the timer in 16-bit mode
#define TIMER_INT_TYPE uint16_t
#define TIMER_INT_SIZE 16

/// The clock generator number to use
#define GENERIC_CLOCK_GENERATOR_SDI12 (6u)
/// The bit to check for synchronization
#define GCLK_SYNCBUSY_SDI12 GCLK_SYNCBUSY_GENCTRL6
/// The timer controller to use
#define SDI12_TC TC2
// The peripheral index within the generic clock for the selected timer controller
#define SDI12_TC_GCLK_ID TC2_GCLK_ID

// For the SAMD51, reading the timer is a multi-step process of first writing a read
// sync bit, waiting, and then reading the register.  Because of the steps, we need a
// function.
#define READTIME sdi12timer.SDI12TimerRead()

#define PRESCALE_IN_USE_STR "120MHz/15/16=500kHz"
// Start with 120MHz "main" clock source (MAIN_CLOCK_SOURCE = GCLK_GENCTRL_SRC_DPLL0)
// 120MHz / 15x clock source divider (GCLK_GENCTRL_DIV(15)) = 8MHz
// 8MHz / 16x prescaler (TC_CTRLA_PRESCALER_DIV16) = 500kHz
// 500,000 'ticks'/sec = 2 µs / 'tick' (1 sec/1200 bits) * (1 tick/2 µs) = 416.66667
// ticks/bit
#define TICKS_PER_SECOND 500000

// Espressif ESP32/ESP8266 boards or any boards faster than 48MHz not mentioned above
// WARNING: I haven't actually tested the minimum speed that this will work at!
#elif defined(ESP32) || defined(ESP8266) || F_CPU >= 48000000L

// Using the micros() function
#define TIMER_IN_USE_STR "micros()"
// Since we're using `micros()`, this is 32 bit
#define TIMER_INT_TYPE uint32_t
#define TIMER_INT_SIZE 32
#define READTIME sdi12timer.SDI12TimerRead()
// Since we're using micros() each 'tick' is 1µs
#define TICKS_PER_SECOND 1000000

// Unknown board
#else
#error "Please define your board timer and prescaler!"
#endif


#if TICKS_PER_SECOND == 15625 && TIMER_INT_SIZE == 8
/**
 * 15625 'ticks'/sec = 64 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
 *
 * The 8-bit timer rolls over after 256 ticks, 19.66085 bits, or 16.38505 ms
 * (256 ticks/roll-over) * (1 bit/13.0208 ticks) = 19.66085 bits
 * (256 ticks/roll-over) * (1 sec/15625 ticks) = 16.38505 milliseconds
 */
#define TICKS_PER_BIT 13
/**
 * 1/(13.0208 ticks/bit) * 2^10 = 78.6432
 */
#define BITS_PER_TICK_Q10 79
#define RX_WINDOW_FUDGE 2

#elif TICKS_PER_SECOND == 11719 && TIMER_INT_SIZE == 8
/**
 * 11719 'ticks'/sec = 85 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/85 µs) = 9.765625 ticks/bit
 *
 * The 8-bit timer rolls over after 256 ticks, 26.2144 bits, or 21.84487 ms
 * (256 ticks/roll-over) * (1 bit/9.765625 ticks) = 26.2144 bits
 * (256 ticks/roll-over) * (1 sec/11719 ticks) = 21.84487 milliseconds
 */
#define TICKS_PER_BIT 10
/**
 * 1/(9.765625 ticks/bit) * 2^10 = 104.8576
 */
#define BITS_PER_TICK_Q10 105
#define RX_WINDOW_FUDGE 2

#elif TICKS_PER_SECOND == 31250 && TIMER_INT_SIZE == 8
/**
 * 31250 'ticks'/sec = 32 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/32 µs) = 26.04166667 ticks/bit
 *
 * The 8-bit timer rolls over after 256 ticks, 9.8304 bits, or 8.192 ms
 * (256 ticks/roll-over) * (1 bit/26.04166667 ticks) = 9.8304 bits
 * (256 ticks/roll-over) * (1 sec/31250 ticks) = 8.192 milliseconds
 * @note The timer will roll-over with each character!
 */
#define TICKS_PER_BIT 26
/**
 * 1/(26.04166667 ticks/bit) * 2^10 = 39.3216
 */
#define BITS_PER_TICK_Q10 39
#define RX_WINDOW_FUDGE 10


#elif TICKS_PER_SECOND == 500000 && TIMER_INT_SIZE == 16
/**
 * 500kHz = 500,000 'ticks'/sec = 2 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/2 µs) = 416.66667 ticks/bit
 *
 * The 16-bit timer rolls over after 65536 ticks, 157.284 bits, or 131.07 ms
 * (65536 ticks/roll-over) * (1 bit/416.66667 ticks) = 157.284 bits
 * (65536 ticks/roll-over) * (1 sec/500000 ticks) = 131.07 milliseconds
 */
#define TICKS_PER_BIT 416
#define RX_WINDOW_FUDGE 30

#elif TICKS_PER_SECOND == 1000000 && TIMER_INT_SIZE == 32
/**
 * Using `micros()` 1 "tick" is 1 µsec
 * (1 sec/1200 bits) * (1 tick/1 µs) * (1000000 µsec/sec)= 833.33333 ticks/bit
 *
 * The 32-bit timer rolls over after 4294967296 ticks, or 4294.9673 seconds
 */
#define TICKS_PER_BIT 833UL
#define RX_WINDOW_FUDGE 50

#else
#error "Board timer is incorrectly configured!"
#endif


/** The interger type (size) of the timer return value */
typedef TIMER_INT_TYPE sdi12timer_t;

/**
 * @brief The class used to define the processor timer for the SDI-12 serial emulation.
 */
class SDI12Timer {
 public:
  /**
   * @brief Construct a new SDI12Timer
   */
  SDI12Timer();

  /**
   * @brief static method for getting a 16-bit value from the multiplication of 2 8-bit
   * values
   *
   * @param x The first 8 bit integer
   * @param y The second 8 bit integer
   * @return The result of the multiplication, as a 16 bit integer.
   */
  static uint16_t mul8x8to16(uint8_t x, uint8_t y);

  /**
   * @brief static method for calculating the number of bit-times that have elapsed
   * between interrupts.
   *
   * @param dt The current value of the timer
   * @return The number of bit times that have passed at 1200 baud.
   *
   * Adds a rxWindowWidth fudge factor to the time difference to get the number of
   * ticks, and then multiplies the fudged ticks by the number of bits per tick.  Uses
   * the number of bits per tick shifted up by 2^10 and then shifts the result down by
   * the same amount to compensate for the fact that the number of bits per tick is a
   * decimal the timestamp is only an 8-bit integer.
   *
   * @see https://github.com/SlashDevin/NeoSWSerial/pull/13#issuecomment-315463522
   */
  static uint16_t bitTimes(sdi12timer_t dt);

  /**
   * @brief Set the processor timer prescaler such that the 10 bits of an SDI-12
   * character are divided into the rollover time of the timer.
   */
  void configSDI12TimerPrescale(void);
  /**
   * @brief Reset the processor timer prescaler to whatever it was prior to being
   * adjusted for this library.
   */
  void resetSDI12TimerPrescale(void);
  /**
   * @brief A function to read the timer value, where a multi-step function is needed.
   *
   * @return **sdi12timer_t** The current timer value
   */
  sdi12timer_t SDI12TimerRead(void);
};

#endif  // SRC_SDI12_BOARDS_H_
