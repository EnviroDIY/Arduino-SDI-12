/**
 * @file SDI12_boards.cpp
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 * @author Sara Geleskie Damiano (sdamiano@stroudcenter.org)
 *
 * @brief This file implements the setting and unsetting of the proper prescalers for
 * the timers for SDI-12.
 *
 */

/* ======================== Arduino SDI-12 =================================
An Arduino library for SDI-12 communication with a wide variety of environmental
sensors. This library provides a general software solution, without requiring
   ======================== Arduino SDI-12 =================================*/

#include "SDI12_boards.h"

SDI12Timer::SDI12Timer() {}

// Most 'standard' AVR boards
//
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || \
  defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) ||  \
  defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) ||   \
  defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)

/**
 * @brief The value of timer control register 2A prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR2A;
/**
 * @brief The value of timer control register 2B prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR2B;
/**
 * @brief The output compare match value
 */
static uint8_t preSDI12_OCR2A;

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT2;  // Using Timer 2
}

void SDI12Timer::revertSDI12Timer(void) {
  TCCR2A = preSDI12_TCCR2A;
  TCCR2B = preSDI12_TCCR2B;
  OCR2A  = preSDI12_OCR2A;
  disableSDI12TimerInterrupt();
}

void SDI12Timer::resetSDI12TimerValue(void) {
  TCNT2 = 0;
}

void SDI12Timer::enableSDI12TimerInterrupt(void) {
  TIMSK2 |= 0x02;  // TIMSK2 |= 0x02 = 0b00000010 enable Bit 1 – OCIEA: Timer/Counter2,
                   // Output Compare A Match Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::disableSDI12TimerInterrupt(void) {
  TIMSK2 &= 0xFD;  // TIMSK2 |= 0x02 = 0b11111101 enable Bit 1 – OCIEA: Timer/Counter2,
                   // Output Compare A Match Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::clearSDI12TimerInterrupt(void) {
  // Bit 1 – OCFA: Timer/Counter2, Output Compare A Match Flag
  // The OCFA bit is set (one) when a compare match occurs between the Timer/Counter2
  // and the data in OCRA – Output Compare Register2. OCFA is cleared by hardware when
  // executing the corresponding interrupt handling vector. Alternatively, OCFA is
  // cleared by writing a logic one to the flag. When the I-bit in SREG, OCIEA
  // (Timer/Counter2 Compare match Interrupt Enable), and OCFA are set (one), the
  // Timer/Counter2 Compare match Interrupt is executed.
  TIFR2 = 0x02;  // TIFR2 = 0x02 = 0b00000010 enable Bit 1 – OCFA: Timer/Counter2,
                 // Output Compare A Match Flag (clears the flag)
}

#if F_CPU == 16000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  preSDI12_OCR2A  = OCR2A;
  TCCR2A          = 0x02;  // TCCR2A = 0x02 = 0b00000010 = CTC without output pins
                           // bits 7:6 unset - OC2A pin disconnected
                           // bits 5:4 unset - OC2B pin disconnected
                           // bits 3:2 - Reserved
                           // bits 1:0 - WGM21 set and WGM20 unset =
                           // Clear Timer on Compare Match (CTC) Mode
  TCCR2B = 0x0F;  // TCCR2B = 0x0F = 0b00001111 - CTC with prescaler set to CK/1024
                  // bits 7:6 unset - do not force output compare A or B
                  // bits 5:4 - Reserved
                  // bit 3 set - WGM22 works with WGM21 and WGM20 to enable CTC mode
                  // bits 2:0 set: Clock Select bits CA22, CA21, & CA20 on - prescaler
                  // set to CK/1024
  OCR2A = TIMER_ROLLOVER_COUNT;  // Output compare register 2A
}

#elif F_CPU == 12000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  preSDI12_OCR2A  = OCR2A;
  TCCR2A          = 0x02;  // TCCR2A = 0x02 = 0b00000010 = CTC without output pins
                           // bits 7:6 unset - OC2A pin disconnected
                           // bits 5:4 unset - OC2B pin disconnected
                           // bits 3:2 - Reserved
                           // bits 1:0 - WGM21 set and WGM20 unset =
                           // Clear Timer on Compare Match (CTC) Mode
  TCCR2B = 0x0F;  // TCCR2B = 0x0F = 0b00001111 - CTC with prescaler set to CK/1024
                  // bits 7:6 unset - do not force output compare A or B
                  // bits 5:4 - Reserved
                  // bit 3 set - WGM22 works with WGM21 and WGM20 to enable CTC mode
                  // bits 2:0 set: Clock Select bits CA22, CA21, & CA20 on - prescaler
                  // set to CK/1024
  OCR2A = TIMER_ROLLOVER_COUNT;  // Output compare register 2A
}

#elif F_CPU == 8000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  preSDI12_OCR2A  = OCR2A;
  TCCR2A          = 0x02;  // TCCR2A = 0x02 = 0b00000010 = CTC without output pins
                           // bits 7:6 unset - OC2A pin disconnected
                           // bits 5:4 unset - OC2B pin disconnected
                           // bits 3:2 - Reserved
                           // bits 1:0 - WGM21 set and WGM20 unset =
                           // Clear Timer on Compare Match (CTC) Mode
  TCCR2B =
    0x0E;  // TCCR2B = 0x0E = 0b00001110 - CTC with prescaler set to CK/256
           // bits 7:6 unset - do not force output compare A or B
           // bits 5:4 - Reserved
           // bit 3 set - WGM22 works with WGM21 and WGM20 to enable CTC mode
           // bits 2:0 set: Clock Select bits CA22 & CA20 on - prescaler set to CK/256
  OCR2A = TIMER_ROLLOVER_COUNT;  // Output compare register 2A
}
#endif


// ATtiny boards (ie, adafruit trinket)
//
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

/**
 * @brief The value of timer control register 1A prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR1A;
/**
 * @brief The output compare match value
 */
static uint8_t preSDI12_OCR1A;

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT1;  // Using Timer 1
}

void SDI12Timer::revertSDI12Timer(void) {
  TCCR1 = preSDI12_TCCR1A;
  OCR1A = preSDI12_OCR1A;
  disableSDI12TimerInterrupt();
}

void SDI12Timer::resetSDI12TimerValue(void) {
  TCNT1 = 0;
}

void SDI12Timer::enableSDI12TimerInterrupt(void) {
  // NOTE:  The same TIMSK and TIFR registers are used for Timer0 and Timer1 on the
  // AtTiny
  TIMSK |= 0x40;  // TIMSK = 0x40 = 0b01000000 Bit 6 - OCIE1A: Timer/Counter1 Output
                  // Compare Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::disableSDI12TimerInterrupt(void) {
  TIMSK &= 0xBF;  // TIMSK = 0xBF = 0b10111111 Bit 6 - OCIE1A: Timer/Counter1 Output
                  // Compare Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::clearSDI12TimerInterrupt(void) {
  TIFR = 0x40;  // TIFR = 0x40 = 0b01000000 enable Bit 6 - OCF1A: Output Compare Flag
                // 1A (clear flag)
}

#if F_CPU == 16000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR1A = TCCR1;
  preSDI12_OCR1A  = OCR1A;
  TCCR1 = 0b10001011;  // Enable clear timer on capture compare match and set the
                       // prescaler to 1024
  // Bit 7 - CTC1: Clear Timer/Counter on Compare Match Enabled
  // Bit 6 - PWM1A: Pulse Width Modulator A Disabled
  // Bits 5,4 - COM1A1, COM1A0: Comparator A Output Mode, Bits 1 and 0 - both 0 disables
  // output Bits 3..0 - CS13, CS12, CS11, CS10: Clock Select Bits 3, 2, 1, and 0,
  // 1011=PCK/1024
  OCR1A = TIMER_ROLLOVER_COUNT;  // Output compare register 1A
  // NOTE:  The same TIMSK and TIFR registers are used for Timer0 and Timer1 on the
  // AtTiny
}


#elif F_CPU == 8000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR1A = TCCR1;
  TCCR1 = 0b10001010;  // Enable clear timer on capture compare match and set the
                       // prescaler to 512
  // Bit 7 - CTC1: Clear Timer/Counter on Compare Match Enabled
  // Bit 6 - PWM1A: Pulse Width Modulator A Disabled
  // Bits 5,4 - COM1A1, COM1A0: Comparator A Output Mode, Bits 1 and 0 - both 0 disables
  // output Bits 3..0 - CS13, CS12, CS11, CS10: Clock Select Bits 3, 2, 1, and 0,
  // 1010=PCK/512
  OCR1A = TIMER_ROLLOVER_COUNT;  // Output compare register 1A
}
#endif


// Arduino Leonardo & Yun and other 32U4 boards
//
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || \
  defined(__AVR_ATmega32U4__)

/**
 * @brief The value of timer control register 4A prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR4A;
/**
 * @brief The value of timer control register 4B prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR4B;
/**
 * @brief The value of timer control register 4C prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR4C;
/**
 * @brief The value of timer control register 4D prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR4D;
/**
 * @brief The value of timer control register 4E prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR4E;
/**
 * @brief The output compare match value
 */
static uint8_t preSDI12_OCR4A;

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT4;  // Using Timer 4
}

void SDI12Timer::revertSDI12Timer(void) {
  TCCR4A = preSDI12_TCCR4A;
  TCCR4B = preSDI12_TCCR4B;
  TCCR4C = preSDI12_TCCR4C;
  TCCR4D = preSDI12_TCCR4D;
  TCCR4E = preSDI12_TCCR4E;
  OCR4A  = preSDI12_OCR4A;
  disableSDI12TimerInterrupt(void);
}

void SDI12Timer::resetSDI12TimerValue(void) {
  TCNT4 = 0;
}

void SDI12Timer::enableSDI12TimerInterrupt(void) {
  TIMSK4 |= 0x40;  // TIMSK4 = 0x40 = 0b01000000 Bit 6 - OCIE1A: Timer/Counter1 Output
                   // Compare Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::disableSDI12TimerInterrupt(void) {
  TIMSK4 &= 0xBF;  // TIMSK4 = 0xBF = 0b10111111 Bit 6 - OCIE1A: Timer/Counter1 Output
                   // Compare Interrupt Enable
  clearSDI12TimerInterrupt();
}

void SDI12Timer::clearSDI12TimerInterrupt(void) {
  // The OCF4A bit is set (one) when compare match occurs between Timer/Counter4 and the
  // data value in OCR4A - Output Compare Register 4A. OCF4A is cleared by hardware when
  // executing the corresponding interrupt handling vector. Alternatively, OCF4A is
  // cleared, after synchronization clock cycle, by writing a logic one to the flag.
  // When the I-bit in SREG, OCIE4A, and OCF4A are set (one), the Timer/Counter4 A
  // compare match interrupt is executed.
  TIFR4 = 0x40;  // TIFR = 0x40 = 0b01000000 enable Bit 6 - OCF1A: Output Compare Flag
                 // 1A (clear flag)
  TIFR4 = 0;  // Also reset the count, since this does NOT happen automatically when the
              // interrupt fires.  Timer 4 doesn't support CTC mode like the AVR boards.
}

#if F_CPU == 16000000L

void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR4A = TCCR4A;
  preSDI12_TCCR4B = TCCR4B;
  preSDI12_TCCR4C = TCCR4C;
  preSDI12_TCCR4D = TCCR4D;
  preSDI12_TCCR4E = TCCR4E;
  preSDI12_OCR4A  = OCR4A;
  TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A &
                  // OC4B disconnected
                  // Bits 7, 6 - COM4A1, COM4A0: Comparator A Output Mode, Bits 1 and 0,
                  // output compare pin disabled
                  // Bits 5,4 - COM4B1, COM4B0: Comparator B Output Mode, Bits 1 and 0,
                  // output compare pin disabled
                  // Bit 3 - FOC4A: Force Output Compare Match 4A, disabled
                  // Bit 2 - FOC4B: Force Output Compare Match 4B, disabled
                  // Bit 1 - PWM4A: Pulse Width Modulator A Enable, disabled
                  // Bit 0 - PWM4B: Pulse Width Modulator B Enable, disabled
  TCCR4B = 0x0B;  // TCCR4B = 0x0B = 0b00001011 - Clock Select bits 43, 41, & 40 on -
                  // prescaler set to CK/1024
                  // Bit 7 - PWM4X: PWM Inversion Mode, disabled
                  // Bit 6 - PSR4: Prescaler Reset Timer/Counter4
                  // Bits 5,4 - DTPS41, DTPS40: Dead Time Prescaler Bits
                  // Bits 3..0 - CS43, CS42, CS41, CS40: Clock Select Bits 3, 2, 1, and
                  // 0 - 1011=CK/1024
  TCCR4C =
    0x00;  // TCCR4C = 0x00 = "normal" operation - with output pins (A-D) disabled
  TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
  TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
  OCR4A  = TIMER_ROLLOVER_COUNT;
}

#elif F_CPU == 8000000L
void SDI12Timer::configureSDI12Timer(void) {
  preSDI12_TCCR4A = TCCR4A;
  preSDI12_TCCR4B = TCCR4B;
  preSDI12_TCCR4C = TCCR4C;
  preSDI12_TCCR4D = TCCR4D;
  preSDI12_TCCR4E = TCCR4E;
  preSDI12_OCR4A  = OCR4A;
  TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A &
                  // OC4B disconnected
  TCCR4B = 0x0A;  // TCCR4B = 0x0A = 0b00001010 - Clock Select bits 43 & 41 on -
                  // prescaler set to CK/512
  TCCR4C = 0x00;  // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0
                  // disconnected
  TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
  TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
  OCR4A  = TIMER_ROLLOVER_COUNT;
}
#endif


// Arduino Zero other SAMD21 boards
//
#elif defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_ARCH_SAMD) || \
  defined(__SAMD21G18A__) || defined(__SAMD21J18A__) || defined(__SAMD21E18A__)


sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return REG_TC3_COUNT8_COUNT;  // Using Timer 3 with generic clock 4
}

void SDI12Timer::configureSDI12Timer(void) {
  // Select generic clock generator 4 (Arduino core uses 0, 1, and 3.  RTCZero uses 2)
  // Many examples use clock generator 4.. consider yourself warned!
  // I would use a higher clock number, but some of the cores don't include them for
  // some reason
  REG_GCLK_GENDIV = GCLK_GENDIV_ID(4) |  // Select Generic Clock Generator 4
    GCLK_GENDIV_DIV(3);                  // Divide the clock source by divisor 3
  while (GCLK->STATUS.bit.SYNCBUSY) {}   // Wait for synchronization


  // Write the generic clock generator 4 configuration
  REG_GCLK_GENCTRL = (GCLK_GENCTRL_ID(4) |        // Select GCLK4
                      GCLK_GENCTRL_SRC_DFLL48M |  // Select the 48MHz clock source
                      GCLK_GENCTRL_IDC |     // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN) &  // Enable the generic clock clontrol
    ~GCLK_GENCTRL_RUNSTDBY &                 // Do NOT run in stand by
    ~GCLK_GENCTRL_DIVSEL;  // Divide clock source by GENDIV.DIV: 48MHz/3=16MHz
                           // ^^ & ~ for DIVSEL because not not divided
  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization

  // Feed GCLK4 to TC3 (also feeds to TCC2, the two must have the same source)
  // TC3 (and TCC2) seem to be free, so I'm using them
  // TC4 is used by Tone, TC5 is tied to the same clock as TC4
  // TC6 and TC7 are not available on all boards
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_GEN_GCLK4 |  // Select Generic Clock Generator 4
    GCLK_CLKCTRL_CLKEN |                       // Enable the generic clock generator
    GCLK_CLKCTRL_ID_TCC2_TC3;  // Feed the Generic Clock Generator 4 to TCC2 and TC3
  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization

  REG_TC3_CTRLA |=
    TC_CTRLA_PRESCALER_DIV1024 |  // Set prescaler to 1024, 16MHz/1024 = 15.625kHz
    TC_CTRLA_WAVEGEN_NFRQ |       // Put the timer TC3 into normal frequency (NFRQ) mode
    TC_CTRLA_MODE_COUNT8 |        // Put the timer TC3 into 8-bit mode
    TC_CTRLA_ENABLE;              // Enable TC3
  while (TC3->COUNT8.STATUS.bit.SYNCBUSY) {}  // Wait for synchronization

  REG_TC3_CTRLC |= TC_CTRLC_CPTEN0;  // enable capture channel 0 on TC3

  REG_TC3_COUNT8_CC0 =
    TIMER_ROLLOVER_COUNT;  // TC3 Channel 0 Compare/Capture Value, 8-bit Mode
  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization
}

// NOT resetting the SAMD timer settings
void SDI12Timer::revertSDI12Timer(void) {
  REG_TC3_INTENCLR |= TC_INTENCLR_MC0;  // Clear the Match or Capture Channel 0
                                        // Interrupt Enable, no sync needed

  // Disable TCx
  TC3->COUNT8.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC3->COUNT8.STATUS.bit.SYNCBUSY) {}

  // Reset TCx
  TC3->COUNT8.CTRLA.reg = TC_CTRLA_SWRST;
  while (TC3->COUNT8.STATUS.bit.SYNCBUSY) {}
  while (TC3->COUNT8.CTRLA.bit.SWRST) {}

  // Disable generic clock generator
  REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4) &  // Select GCLK4
    ~GCLK_GENCTRL_GENEN;                   // Disable the generic clock control
  while (GCLK->STATUS.bit.SYNCBUSY) {}     // Wait for synchronization
}

void SDI12Timer::resetSDI12TimerValue(void) {
  REG_TC3_COUNT8_COUNT = 0;
}

void SDI12Timer::clearSDI12TimerInterrupt(void) {
  TC3->COUNT8.INTFLAG.reg |= TC_INTFLAG_MC0;  // clear MC0
  REG_TC3_COUNT8_COUNT = 0;  // Also reset the count, since this does NOT happen
                             // automatically when the interrupt fires.  The SAMD21
                             // doesn't support CTC mode like the AVR boards.
}

void SDI12Timer::enableSDI12TimerInterrupt(void) {
  REG_TC3_INTENSET |= TC_INTENSET_MC0;  // Match or Capture Channel 0 Interrupt Enable
  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization
  clearSDI12TimerInterrupt();
}

void SDI12Timer::disableSDI12TimerInterrupt(void) {
  REG_TC3_INTENCLR |= TC_INTENCLR_MC0;  // Match or Capture Channel 0 Interrupt Clear
  // NOTE: Don't need to wait for sync bit
  clearSDI12TimerInterrupt();
}

// Espressif ESP32/ESP8266 boards
//
#elif defined(ESP32) || defined(ESP8266)

void SDI12Timer::configureSDI12Timer(void) {}

void SDI12Timer::revertSDI12Timer(void) {}

/**
 * @brief Read the processor micros and right shift 6 bits (ie, divide by 64) to get a
 * 64µs tick.
 *
 * @note  The ESP32 and ESP8266 are fast enough processors that they can take the time
 * to read the core 'micros()' function still complete the other processing needed on
 * the serial bits.  All of the other processors using the Arduino core also have the
 * micros function, but the rest are not fast enough to waste the processor cycles to
 * use the micros function and must use the faster assembly macros to read the
 * processor timer directly.
 *
 * @return **sdi12timer_t** The current processor micros
 */
sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  // Its a one microsecond clock but we want 64uS ticks so divide by 64 i.e. right shift
  // 6
  return ((sdi12timer_t)(micros() >> 6));
}

// Unknown board
#else
#error "Please define your board timer and pins"
#endif
