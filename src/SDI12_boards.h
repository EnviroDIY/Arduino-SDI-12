/* ======================== Arduino SDI-12 =================================
Arduino library for SDI-12 communications to a wide variety of environmental
sensors. This library provides a general software solution, without requiring
   ======================== Arduino SDI-12 =================================*/

#ifndef SDI12_boards_h
#define SDI12_boards_h

#include <Arduino.h>

class SDI12Timer
{
public:
    SDI12Timer();
    void configSDI12TimerPrescale(void);
    void resetSDI12TimerPrescale(void);

// Most 'standard' AVR boards
//
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) || \
    defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)

    #define TIMER_IN_USE_STR "TCNT2"
    #define TCNTX TCNT2  // Using Timer 2

    #if F_CPU == 16000000L
        #define PRESCALE_IN_USE_STR "1024"
        #define TICKS_PER_BIT 13
            // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
            // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
        #define BITS_PER_TICK_Q10 79
            // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
        #define RX_WINDOW_FUDGE 2

    #elif F_CPU == 8000000L
        #define PRESCALE_IN_USE_STR "256"
        #define TICKS_PER_BIT 26
            // 8MHz / 256 prescaler = 31250 'ticks'/sec = 32 µs / 'tick'
            // (1 sec/1200 bits) * (1 tick/32 µs) = 26.04166667 ticks/bit
        #define BITS_PER_TICK_Q10 39
            // 1/(26.04166667 ticks/bit) * 2^10 = 39.3216
        #define RX_WINDOW_FUDGE 10

        // #define PRESCALE_IN_USE_STR "1024"
        // #define TICKS_PER_BIT 6
        //     // 8MHz / 1024 prescaler = 31250 'ticks'/sec = 128 µs / 'tick'
        //     // (1 sec/1200 bits) * (1 tick/128 µs) = 6.5104166667 ticks/bit
        // #define BITS_PER_TICK_Q10 157
        //     // 1/(6.5104166667 ticks/bit) * 2^10 = 157.2864
        // #define RX_WINDOW_FUDGE 5

     #endif


// ATtiny boards (ie, adafruit trinket)
//
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

     #if F_CPU == 16000000L
         #define PRESCALE_IN_USE_STR "1024"
         #define TICKS_PER_BIT 13
         // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
         // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
         // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 2

     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define TICKS_PER_BIT 13
         // 8MHz / 512 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
         // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
         // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 5

      #endif


// Arduino Leonardo & Yun and other 32U4 boards
//
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega32U4__)

    #define TIMER_IN_USE_STR "TCNT4"
    #define TCNTX TCNT4  // Using Timer 4

     #if F_CPU == 16000000L
         #define PRESCALE_IN_USE_STR "1024"
         #define TICKS_PER_BIT 13
             // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
             // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
             // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 2

     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define TICKS_PER_BIT 13
             // 8MHz / 512 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
             // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
             // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 5

      #endif


// Arduino Zero other SAMD21 boards
//
#elif defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_ARCH_SAMD) || \
      defined(__SAMD21G18A__) || defined(__SAMD21J18A__) || defined(__SAMD21E18A__)

    #define TIMER_IN_USE_STR "GCLK4-TC4"
    #define TCNTX REG_TC4_COUNT8_COUNT  // Using Timer 4

    #define PRESCALE_IN_USE_STR "3x1024"
    #define TICKS_PER_BIT 13
        // 48MHz / 3 pre-prescaler = 16MHz
        // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
        // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
    #define BITS_PER_TICK_Q10 79
        // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
    #define RX_WINDOW_FUDGE 2

// Unknown board
#else
#error "Please define your board timer and pins"
#endif

};

#endif
