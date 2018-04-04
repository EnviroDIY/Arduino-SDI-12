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
        #define CONFIG_TIMER_PRESCALE() (TCCR2A = 0x00, TCCR2B = 0x07)
            // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A & OC2B disconnected
            // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on - prescaler set to CK/1024
        #define TICKS_PER_BIT 13
            // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
            // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
        #define BITS_PER_TICK_Q10 79
            // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
        #define RX_WINDOW_FUDGE 2
    #elif F_CPU == 8000000L
        // #define PRESCALE_IN_USE_STR "256"
        // #define CONFIG_TIMER_PRESCALE() (TCCR2A = 0x00, TCCR2B = 0x06)
        //     // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
        //     // TCCR2B = 0x06 = 0b00000110 - Clock Select bits 22 & 20 on - prescaler set to CK/256
        // #define TICKS_PER_BIT 26
        //     // 8MHz / 256 prescaler = 31250 'ticks'/sec = 32 µs / 'tick'
        //     // (1 sec/1200 bits) * (1 tick/32 µs) = 26.04166667 ticks/bit
        // #define BITS_PER_TICK_Q10 39
        //     // 1/(26.04166667 ticks/bit) * 2^10 = 39.3216
        // #define RX_WINDOW_FUDGE 10
        #define PRESCALE_IN_USE_STR "1024"
        #define CONFIG_TIMER_PRESCALE() (TCCR2A = 0, TCCR2B = 0x07)  // Set the prescaler to 1024
            // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
            // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on - prescaler set to CK/1024
        #define TICKS_PER_BIT 6
            // 8MHz / 1024 prescaler = 31250 'ticks'/sec = 128 µs / 'tick'
            // (1 sec/1200 bits) * (1 tick/128 µs) = 6.5104166667 ticks/bit
        #define BITS_PER_TICK_Q10 157
            // 1/(6.5104166667 ticks/bit) * 2^10 = 157.2864
        #define RX_WINDOW_FUDGE 5
     #endif


// ATtiny boards (ie, adafruit trinket)
//
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

    #define TIMER_IN_USE_STR "TCNT1"
    #define TCNTX TCNT1  // Using Timer 1

     #if F_CPU == 16000000L
         #define PRESCALE_IN_USE_STR "1024"
         #define CONFIG_TIMER_PRESCALE() (TCCR1A = 0b00001011)  // Set the prescaler to 1024
         #define TICKS_PER_BIT 13
         // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
         // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
         // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 2
     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define CONFIG_TIMER_PRESCALE() (TCCR1A = 0b00001010)  // Set the prescaler to 512
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
         #define CONFIG_TIMER_PRESCALE() (TCCR4A = 0x00, TCCR4B = 0x0B, TCCR4C = 0x00, TCCR4D = 0x00, TCCR4E = 0x00)
              // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
              // TCCR4B = 0x0B = 0b00001011 - Clock Select bits 43, 41, & 40 on - prescaler set to CK/1024
              // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0 disconnected
              // TCCR4D = 0x00 = No fault protection
              // TCCR4E = 0x00 = No register locks or overrides
         #define TICKS_PER_BIT 13
             // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
             // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
             // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 2
     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define CONFIG_TIMER_PRESCALE() (TCCR4A = 0x00, TCCR4B = 0x0A, TCCR4C = 0x00, TCCR4D = 0x00, TCCR4E = 0x00)
              // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
              // TCCR4B = 0x0A = 0b00001010 - Clock Select bits 43 & 41 on - prescaler set to CK/512
              // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0 disconnected
              // TCCR4D = 0x00 = No fault protection
              // TCCR4E = 0x00 = No register locks or overrides
         #define TICKS_PER_BIT 13
             // 8MHz / 512 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
             // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
             // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 5
      #endif


// Arduino Zero other SAMD21 boards
//
#elif defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_ARCH_SAMD) || defined(__SAMD21G18A__) || defined(__SAMD21J18A__)

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
