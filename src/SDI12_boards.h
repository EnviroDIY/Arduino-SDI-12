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

#if defined(ESP32) || defined(ESP8266)
// On espressif boards (ESP8266 and ESP32), the ISR must be stored in IRAM
#define ESPFAMILY_USE_INSTRUCTION_RAM IRAM_ATTR
#else
#define ESPFAMILY_USE_INSTRUCTION_RAM
#endif


// Most 'standard' AVR boards
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || \
  defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) ||  \
  defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) ||   \
  defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)

/**
 * @brief A string description of the timer to use
 */
#define TIMER_IN_USE_STR "Timer2"

/**
 * @brief The interger type of the timer.
 *
 * Timer 2 is an 8-bit timer
 */
#define TIMER_INT_TYPE uint8_t
#define TIMER_INT_SIZE 8

/**
 * @brief The function or macro used to read the clock timer value.
 *
 * The c macro name for the register used to access the timer/counter 2 value is TCNT2
 */
#define READTIME TCNT2

#if F_CPU == 16000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 16MHz / 1024 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
 */
#define TICKS_PER_SECOND 15625

#elif F_CPU == 12000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "12MHz/1024=11.7kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 12MHz / 1024 prescaler = 11719 'ticks'/sec = 85.33 µs / 'tick'
 */
#define TICKS_PER_SECOND 11719

#elif F_CPU == 8000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "8MHz/256=31.25kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 8MHz / 256 prescaler = 31250 'ticks'/sec = 32 µs / 'tick'
 */
#define TICKS_PER_SECOND 31250
#endif


// ATtiny boards (ie, adafruit trinket)
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

/**
 * @brief A string description of the timer to use
 */
#define TIMER_IN_USE_STR "Timer1"
/**
 * @brief The interger type of the timer.
 *
 * Timer 1 is an 8-bit timer
 */
#define TIMER_INT_TYPE uint8_t
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 */
#define TIMER_INT_SIZE 8

/**
 * @brief The function or macro used to read the clock timer value.
 *
 * The c macro name for the register used to access the timer/counter 1 value is TCNT1
 */
#define READTIME TCNT1  // Using Timer 1

#if F_CPU == 16000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 16MHz / 1024 prescaler = 15625 'ticks'/sec = 15.625 kHz = 64 µs / 'tick'
 */
#define TICKS_PER_SECOND 15625

#elif F_CPU == 8000000L
#define PRESCALE_IN_USE_STR "8MHz/512=15.625kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 8MHz / 512 prescaler = 15625 'ticks'/sec = 15.625 kHz = 64 µs / 'tick'
 */
#define TICKS_PER_SECOND 15625

#endif


// Arduino Leonardo & Yun and other 32U4 boards
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || \
  defined(__AVR_ATmega32U4__)

/**
 * @brief A string description of the timer to use
 */
#define TIMER_IN_USE_STR "Timer4"

/**
 * @brief The interger type of the timer.
 *
 * Timer 4 is an 10-bit timer, but we're only using the lower 8 bits
 */
#define TIMER_INT_TYPE uint8_t
#define TIMER_INT_SIZE 8
/**
 * @brief The function or macro used to read the clock timer value.
 *
 * The c macro name for the register used to access the timer/counter 4 value is TCNT4.
 *
 * @note We only utilize the low byte register, effectively using the 10-bit timer as an
 * 8-bit timer.
 */
#define READTIME TCNT4

#if F_CPU == 16000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "16MHz/1024=15.625kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 16MHz / 1024 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
 */
#define TICKS_PER_SECOND 15625

#elif F_CPU == 8000000L
/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "8MHz/512=15.625kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * 8MHz / 512 prescaler = 15625 'ticks'/sec = 64 µs / 'tick'
 */
#define TICKS_PER_SECOND 15625

#endif


// Arduino Zero other SAMD21 boards
#elif defined(ARDUINO_SAMD_ZERO) || defined(__SAMD21G18A__) || \
  defined(__SAMD21J18A__) || defined(__SAMD21E18A__)

/**
 * @brief A string description of the timer to use
 *
 * For SDI-12, we'll use generic clock generator 4
 *
 * The Adafruit Arduino core uses:
 * - 0 as GENERIC_CLOCK_GENERATOR_MAIN (the main clock)
 *
 * For SDI-12, we'll use Timer Control 3
 * The Adafruit Arduino core uses:
 * - TC5 for Tone
 * - TC4 for Servo
 */
#define TIMER_IN_USE_STR "GCLK4-TC3"

/**
 * @brief The interger type of the timer.
 *
 * We're using the timer in 16-bit mode
 */
#define TIMER_INT_TYPE uint16_t
#define TIMER_INT_SIZE 16

/**
 * @brief The function or macro used to read the clock timer value.
 *
 * This signifies the register of timer/counter 3, the 16-bit count, the count value
 * This is equivalent to TC3->COUNT16.COUNT.reg
 */
#define READTIME REG_TC3_COUNT16_COUNT

/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "48MHz/6/16=500kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * Start with 48MHz "main" clock source (GCLK_GENCTRL_SRC_DFLL48M)
 * 48MHz / 6x clock source divider (GCLK_GENDIV_DIV(6)) = 8MHz
 * 8MHz / 16x prescaler (TC_CTRLA_PRESCALER_DIV16) =  500kHz = 500,000 'ticks'/sec = 2
 * µs / 'tick' (1 sec/1200 bits) * (1 tick/2 µs) = 416.66667 ticks/bit
 */
#define TICKS_PER_SECOND 500000


// SAMD51 and SAME51 boards
#elif defined(__SAMD51__) || defined(__SAME51__)

/**
 * @brief A string description of the timer to use
 *
 * For SDI-12, we'll use generic clock generator 6
 * The Adafruit Arduino core uses:
 * - 0 as GENERIC_CLOCK_GENERATOR_MAIN
 * - 1 as GENERIC_CLOCK_GENERATOR_48M
 * - 2 as GENERIC_CLOCK_GENERATOR_100M
 * - 3 as GENERIC_CLOCK_GENERATOR_XOSC32K
 * - 4 as GENERIC_CLOCK_GENERATOR_12M
 *
 * For SDI-12, we'll use Timer Control 2
 * The Adafruit Arduino core uses:
 * - TC0 as primary for Tone (any other timer could be used, depending on the pin)
 * - TC1 for Servo (any other timer could be used, depending on the pin)

 */
#define TIMER_IN_USE_STR "GCLK6-TC2"

/**
 * @brief The interger type of the timer.
 *
 * We're using the timer in 16-bit mode
 */
#define TIMER_INT_TYPE uint16_t
#define TIMER_INT_SIZE 16

/**
 * @brief The function or macro used to read the clock timer value.
 *
 * For the SAMD51, reading the timer is a multi-step process of first writing a read
 * sync bit, waiting, and then reading the register.  Because of the steps, we need a
 * function.
 */
#define READTIME sdi12timer.SDI12TimerRead()

/**
 * @brief A string description of the prescaler in use.
 */
#define PRESCALE_IN_USE_STR "120MHz/15/16=500kHz"
/**
 * @brief The number of clock ticks per second, after accounting for the prescaler.
 *
 * Start with 120MHz "main" clock source (MAIN_CLOCK_SOURCE = GCLK_GENCTRL_SRC_DPLL0)
 * 120MHz / 15x clock source divider (GCLK_GENCTRL_DIV(15)) = 8MHz
 * 8MHz / 16x prescaler (TC_CTRLA_PRESCALER_DIV16) = 500kHz = 500,000 'ticks'/sec = 2 µs
 * / 'tick' (1 sec/1200 bits) * (1 tick/2 µs) = 416.66667 ticks/bit
 */
#define TICKS_PER_SECOND 500000

// Espressif ESP32/ESP8266 boards or any boards faster than 48MHz not mentioned above

// From calculations using https://github.com/SRGDamia1/avrcycle, the micros() function
// takes 60 (!!) clock cycles. We're going to blindly assume that the micros() function
// takes up about the same number of clock cycles for all Arduino boards.  This is
// probably a huge assumption, but go with it. If we're going to use micros() for
// timing, lets set a minimum usable CPU speed of the micros() function being accurate
// to 1µs. That means we need to get 60 ticks/1µs or 60MHz. Ehh.. Maybe we'll be
// generous and try it down to 48MHz.
// TODO: Test 48MHz

// I know from testing, that we *cannot* use micros on a board 8MHz AVR board, but that
// it does work on a 80MHz Espressif8266.

// WARNING: I haven't actullay tested the minimum speed that this will work at!
#elif defined(ESP32) || defined(ESP8266) || F_CPU >= 48000000L

/**
 * @brief A string description of the timer to use
 */
#define TIMER_IN_USE_STR "micros"

/**
 * @brief The interger type of the timer.
 *
 * Since we're using `micros()`, this is 32 bit
 */
#define TIMER_INT_TYPE uint32_t
#define TIMER_INT_SIZE 32

#define TICKS_PER_SECOND 1000000

/**
 * @brief The function or macro used to read the clock timer value.
 */
#define READTIME sdi12timer.SDI12TimerRead()

// Unknown board
#else
#error "Please define your board timer and prescaler!"
#endif


#if TICKS_PER_SECOND == 15625 && TIMER_INT_SIZE == 8
/**
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
 * 15625 'ticks'/sec = 64 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
 *
 * The 8-bit timer rolls over after 256 ticks, 19.66085 bits, or 16.38505 ms
 * (256 ticks/roll-over) * (1 bit/13.0208 ticks) = 19.66085 bits
 * (256 ticks/roll-over) * (1 sec/15625 ticks) = 16.38505 milliseconds
 */
#define TICKS_PER_BIT 13
/**
 * @brief The number of "ticks" of the timer per SDI-12 bit, shifted by 2^10.
 *
 * 1/(13.0208 ticks/bit) * 2^10 = 78.6432
 */
#define BITS_PER_TICK_Q10 79
/**
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */
#define RX_WINDOW_FUDGE 2

#elif TICKS_PER_SECOND == 11719 && TIMER_INT_SIZE == 8
/**
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
 * 11719 'ticks'/sec = 85 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/85 µs) = 9.765625 ticks/bit
 *
 * The 8-bit timer rolls over after 256 ticks, 26.2144 bits, or 21.84487 ms
 * (256 ticks/roll-over) * (1 bit/9.765625 ticks) = 26.2144 bits
 * (256 ticks/roll-over) * (1 sec/11719 ticks) = 21.84487 milliseconds
 */
#define TICKS_PER_BIT 10
/**
 * @brief The number of "ticks" of the timer per SDI-12 bit, shifted by 2^10.
 *
 * 1/(9.765625 ticks/bit) * 2^10 = 104.8576
 */
#define BITS_PER_TICK_Q10 105
/**
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */
#define RX_WINDOW_FUDGE 1


#elif TICKS_PER_SECOND == 31250 && TIMER_INT_SIZE == 8
/**
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
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
 * @brief The number of "ticks" of the timer per SDI-12 bit, shifted by 2^10.
 *
 * 1/(26.04166667 ticks/bit) * 2^10 = 39.3216
 */
#define BITS_PER_TICK_Q10 39
/**
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */
#define RX_WINDOW_FUDGE 10


#elif TICKS_PER_SECOND == 500000 && TIMER_INT_SIZE == 16
/**
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
 * 500kHz = 500,000 'ticks'/sec = 2 µs / 'tick'
 * (1 sec/1200 bits) * (1 tick/2 µs) = 416.66667 ticks/bit
 *
 * The 16-bit timer rolls over after 65536 ticks, 157.284 bits, or 131.07 ms
 * (65536 ticks/roll-over) * (1 bit/416.66667 ticks) = 157.284 bits
 * (65536 ticks/roll-over) * (1 sec/500000 ticks) = 131.07 milliseconds
 */
#define TICKS_PER_BIT 417
/**
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */
#define RX_WINDOW_FUDGE 11

#elif TICKS_PER_SECOND == 1000000 && TIMER_INT_SIZE == 32
/**
 * @brief The number of "ticks" of the timer that occur within the timing of one bit at
 * the SDI-12 baud rate of 1200 bits/second.
 *
 * Using `micros()` 1 "tick" is 1 µsec
 * (1 sec/1200 bits) * (1 tick/1 µs) * (1000000 µsec/sec)= 833.33333 ticks/bit
 *
 * The 32-bit timer rolls over after 4294967296 ticks, or 4294.9673 seconds
 */
#define TICKS_PER_BIT 834
/**
 * @brief A "fudge factor" to get the Rx to work well. It mostly works to ensure that
 * uneven tick increments get rounded up.
 *
 * @see https://github.com/SlashDevin/NeoSWSerial/pull/13
 */
#define RX_WINDOW_FUDGE 1

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
  static sdi12timer_t bitTimes(sdi12timer_t dt);

  /**
   * @brief Set the processor timer prescaler such that the 10 bits of an SDI-12
   * character are divided into the rollover time of the timer.
   */
  void configSDI12TimerPrescale(void);
  /**
   * @brief Reset the processor timer prescaler to whatever it was prior to being
   * adjusted for this library.
   *
   * @note The prescaler is *NOT* set back to initial values for SAMD boards!  On those
   * processors.
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
