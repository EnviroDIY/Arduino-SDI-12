#include <Arduino.h>

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

        static uint8_t preSDI12_TCCR2A;
        static uint8_t preSDI12_TCCR2B;
        void configSDI12TimerPrescale(void)
        {
            preSDI12_TCCR2A = TCCR2A;
            preSDI12_TCCR2B = TCCR2B;
            TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A & OC2B disconnected
            TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on - prescaler set to CK/1024
        }
        void resetSDI12TimerPrescale(void)
        {
            TCCR2A = preSDI12_TCCR2A;
            TCCR2B = preSDI12_TCCR2B;
        }


    #elif F_CPU == 8000000L
        #define PRESCALE_IN_USE_STR "256"
        #define TICKS_PER_BIT 26
            // 8MHz / 256 prescaler = 31250 'ticks'/sec = 32 µs / 'tick'
            // (1 sec/1200 bits) * (1 tick/32 µs) = 26.04166667 ticks/bit
        #define BITS_PER_TICK_Q10 39
            // 1/(26.04166667 ticks/bit) * 2^10 = 39.3216
        #define RX_WINDOW_FUDGE 10

        static uint8_t preSDI12_TCCR2A;
        static uint8_t preSDI12_TCCR2B;
        void configSDI12TimerPrescale(void)
        {
            preSDI12_TCCR2A = TCCR2A;
            preSDI12_TCCR2B = TCCR2B;
            TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A & OC2B disconnected
            TCCR2B = 0x06;  // TCCR2B = 0x06 = 0b00000110 - Clock Select bits 22 & 20 on - prescaler set to CK/256
        }
        void resetSDI12TimerPrescale(void)
        {
            TCCR2A = preSDI12_TCCR2A;
            TCCR2B = preSDI12_TCCR2B;
        }

        // #define PRESCALE_IN_USE_STR "1024"
        // #define TICKS_PER_BIT 6
        //     // 8MHz / 1024 prescaler = 31250 'ticks'/sec = 128 µs / 'tick'
        //     // (1 sec/1200 bits) * (1 tick/128 µs) = 6.5104166667 ticks/bit
        // #define BITS_PER_TICK_Q10 157
        //     // 1/(6.5104166667 ticks/bit) * 2^10 = 157.2864
        // #define RX_WINDOW_FUDGE 5

        // uint8_t preSDI12_TCCR2A;
        // uint8_t preSDI12_TCCR2B;
        // void configSDI12TimerPrescale(void)
        // {
        //     preSDI12_TCCR2A = TCCR2A;
        //     preSDI12_TCCR2B = TCCR2B;
        //     TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A & OC2B disconnected
        //     TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on - prescaler set to CK/1024
        // }
        // void resetSDI12TimerPrescale(void)
        // {
        //     TCCR2A = preSDI12_TCCR2A;
        //     TCCR2B = preSDI12_TCCR2B;
        // }
     #endif


// ATtiny boards (ie, adafruit trinket)
//
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

    #define TIMER_IN_USE_STR "TCNT1"
    #define TCNTX TCNT1  // Using Timer 1

     #if F_CPU == 16000000L
         #define PRESCALE_IN_USE_STR "1024"
         #define TICKS_PER_BIT 13
         // 16MHz / 1024 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
         // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
         // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 2

         static uint8_t preSDI12_TCCR1A;
         void configSDI12TimerPrescale(void)
         {
              preSDI12_TCCR1A = TCCR1A
              TCCR1A = 0b00001011  // Set the prescaler to 1024
         }
         void resetSDI12TimerPrescale(void)
         {
             TCCR1A = preSDI12_TCCR1A;
         }


     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define TICKS_PER_BIT 13
         // 8MHz / 512 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
         // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
         // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 5

         static uint8_t preSDI12_TCCR1A;
         void configSDI12TimerPrescale(void)
         {
              preSDI12_TCCR1A = TCCR1A
              TCCR1A = 0b00001010  // Set the prescaler to 512
         }
         void resetSDI12TimerPrescale(void)
         {
             TCCR1A = preSDI12_TCCR1A;
         }
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

         static uint8_t preSDI12_TCCR4A;
         static uint8_t preSDI12_TCCR4B;
         static uint8_t preSDI12_TCCR4C;
         static uint8_t preSDI12_TCCR4D;
         static uint8_t preSDI12_TCCR4E;
         void configSDI12TimerPrescale(void)
         {
             preSDI12_TCCR4A = TCCR4A;
             preSDI12_TCCR4B = TCCR4B;
             preSDI12_TCCR4C = TCCR4C;
             preSDI12_TCCR4D = TCCR4D;
             preSDI12_TCCR4E = TCCR4E;
             TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
             TCCR4B = 0x0B;  // TCCR4B = 0x0B = 0b00001011 - Clock Select bits 43, 41, & 40 on - prescaler set to CK/1024
             TCCR4C = 0x00;  // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0 disconnected
             TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
             TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
         }
         void resetSDI12TimerPrescale(void)
         {
             TCCR4A = preSDI12_TCCR4A;
             TCCR4B = preSDI12_TCCR4B;
             TCCR4C = preSDI12_TCCR4C;
             TCCR4D = preSDI12_TCCR4D;
             TCCR4E = preSDI12_TCCR4E;
         }


     #elif F_CPU == 8000000L
         #define PRESCALE_IN_USE_STR "512"
         #define TICKS_PER_BIT 13
             // 8MHz / 512 prescaler = 15624 'ticks'/sec = 64 µs / 'tick'
             // (1 sec/1200 bits) * (1 tick/64 µs) = 13.0208 ticks/bit
         #define BITS_PER_TICK_Q10 79
             // 1/(13.0208 ticks/bit) * 2^10 = 78.6432
         #define RX_WINDOW_FUDGE 5

         static uint8_t preSDI12_TCCR4A;
         static uint8_t preSDI12_TCCR4B;
         static uint8_t preSDI12_TCCR4C;
         static uint8_t preSDI12_TCCR4D;
         static uint8_t preSDI12_TCCR4E;
         void configSDI12TimerPrescale(void)
         {
             preSDI12_TCCR4A = TCCR4A;
             preSDI12_TCCR4B = TCCR4B;
             preSDI12_TCCR4C = TCCR4C;
             preSDI12_TCCR4D = TCCR4D;
             preSDI12_TCCR4E = TCCR4E;
             TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A & OC4B disconnected
             TCCR4B = 0x0A;  // TCCR4B = 0x0A = 0b00001010 - Clock Select bits 43 & 41 on - prescaler set to CK/512
             TCCR4C = 0x00;  // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0 disconnected
             TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
             TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
         }
         void resetSDI12TimerPrescale(void)
         {
             TCCR4A = preSDI12_TCCR4A;
             TCCR4B = preSDI12_TCCR4B;
             TCCR4C = preSDI12_TCCR4C;
             TCCR4D = preSDI12_TCCR4D;
             TCCR4E = preSDI12_TCCR4E;
         }
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


    void configSDI12TimerPrescale(void)
    {
        // Select generic clock generator 4 (Arduino core uses 0-3)
        // Most examples use this clock generator.. consider yourself warned!
        // I would use a higher clock number, but some of the cores don't include them for some reason
        REG_GCLK_GENDIV = GCLK_GENDIV_ID(4) |           // Select Generic Clock Generator 4
                          GCLK_GENDIV_DIV(3) ;          // Divide the 48MHz clock source by divisor 3: 48MHz/3=16MHz
        while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

        // Write the generic clock generator 5 configuration
        REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4) |         // Select GCLK4
                           GCLK_GENCTRL_SRC_DFLL48M |   // Set the 48MHz clock source
                           GCLK_GENCTRL_IDC |           // Set the duty cycle to 50/50 HIGH/LOW
                           GCLK_GENCTRL_GENEN;          // Enable the generic clock clontrol
        while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

        // Feed GCLK4 to TC4 (also feeds to TC5, the two must have the same source)
        REG_GCLK_CLKCTRL = GCLK_CLKCTRL_GEN_GCLK4 |     // Select Generic Clock Generator 4
                           GCLK_CLKCTRL_CLKEN |         // Enable the generic clock generator
                           GCLK_CLKCTRL_ID_TC4_TC5;     // Feed the Generic Clock Generator 4 to TC4 and TC5
        while (GCLK->STATUS.bit.SYNCBUSY);              // Wait for synchronization

        REG_TC4_CTRLA |= TC_CTRLA_PRESCALER_DIV1024 |   // Set prescaler to 1024, 16MHz/1024 = 15.625kHz
                         TC_CTRLA_WAVEGEN_NFRQ |        // Put the timer TC4 into normal frequency (NFRQ) mode
                         TC_CTRLA_MODE_COUNT8 |         // Put the timer TC4 into 8-bit mode
                         TC_CTRLA_ENABLE;               // Enable TC4
        while (TC4->COUNT16.STATUS.bit.SYNCBUSY);       // Wait for synchronization
    }
    void resetSDI12TimerPrescale(void){}  // NOT resetting the SAMD timers

// Unknown board
#else
#error "Please define your board timer and pins"
#endif
