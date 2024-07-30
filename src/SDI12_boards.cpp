/**
 * @file SDI12_boards.cpp
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
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

uint16_t SDI12Timer::mul8x8to16(uint8_t x, uint8_t y) {
  return x * y;
}

// Using an 8-bit timer, we need to do fanciness to get proper 16 bit results
#if TIMER_INT_SIZE == 8
sdi12timer_t SDI12Timer::bitTimes(sdi12timer_t dt) {
  // multiply the time delta in ticks by the bits per tick
  return mul8x8to16(dt + RX_WINDOW_FUDGE, BITS_PER_TICK_Q10) >> 10;
}

// But nothing fancy for bigger timers
#elif TIMER_INT_SIZE == 16 || TIMER_INT_SIZE == 32
sdi12timer_t SDI12Timer::bitTimes(sdi12timer_t dt) {
  // divide the number of ticks by the ticks per bit
  return (dt + RX_WINDOW_FUDGE) / TICKS_PER_BIT;
}
#else
#error "Board timer is incorrectly configured!"
#endif


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

#if F_CPU == 16000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on -
                  // prescaler set to CK/1024
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR2A = preSDI12_TCCR2A;
  TCCR2B = preSDI12_TCCR2B;
}

#elif F_CPU == 12000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on -
                  // prescaler set to CK/1024
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR2A = preSDI12_TCCR2A;
  TCCR2B = preSDI12_TCCR2B;
}

#elif F_CPU == 8000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x06;  // TCCR2B = 0x06 = 0b00000110 - Clock Select bits 22 & 20 on -
                  // prescaler set to CK/256
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR2A = preSDI12_TCCR2A;
  TCCR2B = preSDI12_TCCR2B;
}

// void SDI12Timer::configSDI12TimerPrescale(void)
// {
//     preSDI12_TCCR2A = TCCR2A;
//     preSDI12_TCCR2B = TCCR2B;
//     TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation,
//     OC2A & OC2B disconnected TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock
//     Select bits 22, 21, & 20 on - prescaler set to CK/1024
// }
// void SDI12Timer::resetSDI12TimerPrescale(void)
// {
//     TCCR2A = preSDI12_TCCR2A;
//     TCCR2B = preSDI12_TCCR2B;
// }
#endif


// ATtiny boards (ie, adafruit trinket)
//
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

/**
 * @brief The value of timer control register 1A prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR1A;

#if F_CPU == 16000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR1A = TCCR1;
  TCCR1           = 0b00001011;  // Set the prescaler to 1024
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR1 = preSDI12_TCCR1A;
}


#elif F_CPU == 8000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR1A = TCCR1;
  TCCR1           = 0b00001010;  // Set the prescaler to 512
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR1 = preSDI12_TCCR1A;
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

#if F_CPU == 16000000L

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR4A = TCCR4A;
  preSDI12_TCCR4B = TCCR4B;
  preSDI12_TCCR4C = TCCR4C;
  preSDI12_TCCR4D = TCCR4D;
  preSDI12_TCCR4E = TCCR4E;
  TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A &
                  // OC4B disconnected
  TCCR4B = 0x0B;  // TCCR4B = 0x0B = 0b00001011 - Clock Select bits 43, 41, & 40 on -
                  // prescaler set to CK/1024
  TCCR4C = 0x00;  // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0
                  // disconnected
  TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
  TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR4A = preSDI12_TCCR4A;
  TCCR4B = preSDI12_TCCR4B;
  TCCR4C = preSDI12_TCCR4C;
  TCCR4D = preSDI12_TCCR4D;
  TCCR4E = preSDI12_TCCR4E;
}

#elif F_CPU == 8000000L
void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR4A = TCCR4A;
  preSDI12_TCCR4B = TCCR4B;
  preSDI12_TCCR4C = TCCR4C;
  preSDI12_TCCR4D = TCCR4D;
  preSDI12_TCCR4E = TCCR4E;
  TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A &
                  // OC4B disconnected
  TCCR4B = 0x0A;  // TCCR4B = 0x0A = 0b00001010 - Clock Select bits 43 & 41 on -
                  // prescaler set to CK/512
  TCCR4C = 0x00;  // TCCR4C = 0x00 = "normal" operation - Normal port operation, OC4D0
                  // disconnected
  TCCR4D = 0x00;  // TCCR4D = 0x00 = No fault protection
  TCCR4E = 0x00;  // TCCR4E = 0x00 = No register locks or overrides
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR4A = preSDI12_TCCR4A;
  TCCR4B = preSDI12_TCCR4B;
  TCCR4C = preSDI12_TCCR4C;
  TCCR4D = preSDI12_TCCR4D;
  TCCR4E = preSDI12_TCCR4E;
}
#endif


// Arduino Zero other SAMD21 boards
//
#elif defined(ARDUINO_SAMD_ZERO) || defined(__SAMD21G18A__) || \
  defined(__SAMD21J18A__) || defined(__SAMD21E18A__)

#define SDI12_TC TC3
#define SDI12_TC_IRQn TC3_IRQn
#define SDI12_Handler TC3_Handler

#define WAIT_TC16_REGS_SYNC(x)           \
  while (x->COUNT16.STATUS.bit.SYNCBUSY) \
    ;

static inline void resetTC(Tc* TCx) {
  // Disable TCx
  TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  WAIT_TC16_REGS_SYNC(TCx)

  // Reset TCx
  TCx->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  WAIT_TC16_REGS_SYNC(TCx)
  while (TCx->COUNT16.CTRLA.bit.SWRST)
    ;
}

void SDI12Timer::configSDI12TimerPrescale(void) {
  // Select generic clock generator 4 (Arduino core uses 0, 1, and 3.  RTCZero uses 2)
  // Many examples use clock generator 4.. consider yourself warned!
  // I would use a higher clock number, but some of the cores don't include them for
  // some reason
  REG_GCLK_GENDIV = GCLK_GENDIV_ID(4) |  // Select Generic Clock Generator 4
    GCLK_GENDIV_DIV(5);                  // Divide the clock source by divisor 5
  while (GCLK->STATUS.bit.SYNCBUSY) {}   // Wait for synchronization
  // NOTE: Could write the above as GCLK->GENDIV.reg instead of REG_GCLK_GENDIV


  // Write the generic clock generator 4 configuration
  REG_GCLK_GENCTRL = (GCLK_GENCTRL_ID(4) |        // Select GCLK4
                      GCLK_GENCTRL_SRC_DFLL48M |  // Select the 48MHz clock source
                      GCLK_GENCTRL_IDC |     // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN) &  // Enable the generic clock clontrol
    ~GCLK_GENCTRL_RUNSTDBY &                 // Do NOT run in stand by
    ~GCLK_GENCTRL_DIVSEL;  // Divide clock source by GENDIV.DIV: 48MHz/5=9.6MHz
                           // ^^ & ~ for DIVSEL to set DIVSEL to 0
  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization
  // NOTE: Could write the above as GCLK->GENCTRL.reg instead of REG_GCLK_GENCTRL

  // Feed GCLK4 to TC3 (also feeds to TCC2, the two must have the same source)
  // TC3 (and TCC2) seem to be free, so I'm using them
  // TC4 is used by Tone and Servo, TC5 is tied to the same clock as TC4
  // TC6 and TC7 are not available on all boards
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_GEN_GCLK4 |  // Select Generic Clock Generator 4
    GCLK_CLKCTRL_CLKEN |                       // Enable the generic clock generator
    GCLK_CLKCTRL_ID_TCC2_TC3;  // Feed the Generic Clock Generator 4 to TCC2 and TC3
  // NOTE: Could write the above as GCLK->CLKCTRL.reg instead of REG_GCLK_CLKCTRL

  while (GCLK->STATUS.bit.SYNCBUSY) {}  // Wait for synchronization
  ;

  REG_TC3_CTRLA |=
    TC_CTRLA_PRESCALER_DIV16 |  // Set prescaler to 16, 9.6MHz/16 = 600kHz
    TC_CTRLA_WAVEGEN_NFRQ |     // Put the timer TC3 into normal frequency (NFRQ) mode
    TC_CTRLA_MODE_COUNT16 |     // Put the timer TC3 into 16-bit mode
    TC_CTRLA_ENABLE;            // Enable TC3


  while (TC3->COUNT16.STATUS.bit.SYNCBUSY) {}  // Wait for synchronization
}

// NOT resetting the SAMD timer settings
void SDI12Timer::resetSDI12TimerPrescale(void) {
  resetTC(SDI12_TC);

  // Disable generic clock generator
  REG_GCLK_GENCTRL = GCLK_GENCTRL_ID(4) &  // Select GCLK4
    ~GCLK_GENCTRL_GENEN;                   // Disable the generic clock control
  while (GCLK->STATUS.bit.SYNCBUSY) {}     // Wait for synchronization
}

// SAMD51 and SAME51 boards
#elif defined(__SAMD51__) || defined(__SAME51__)


#define SDI12_TC TC2
#define SDI12_TC_IRQn TC2_IRQn
#define SDI12_TC_GCLK_ID TC2_GCLK_ID
#define SDI12_Handler TC2_Handler

#define WAIT_TC16_REGS_SYNC(x)           \
  while (x->COUNT16.SYNCBUSY.bit.ENABLE) \
    ;

static inline void resetTC(Tc* TCx) {
  // Disable TCx
  TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  WAIT_TC16_REGS_SYNC(TCx)

  // Reset TCx
  TCx->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  WAIT_TC16_REGS_SYNC(TCx)
  while (TCx->COUNT16.CTRLA.bit.SWRST)
    ;
}

void SDI12Timer::configSDI12TimerPrescale(void) {
  // Select generic clock generator 5
  // The Adafruit Arduino core uses:
  // - 0 as GENERIC_CLOCK_GENERATOR_MAIN
  // - 1 as GENERIC_CLOCK_GENERATOR_48M
  // - 2 as GENERIC_CLOCK_GENERATOR_100M
  // - 3 as GENERIC_CLOCK_GENERATOR_XOSC32K
  // - 4 as GENERIC_CLOCK_GENERATOR_12M

  // Select Generic Clock instance
  // The Adafruit Arduino core uses:
  // TC0 for Tone
  // TC1 for Servo

  // Set up the control register for the clock generator
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_SDI12].reg =
    // ^^ select the control register for generic clock 5
    (GCLK_GENCTRL_DIV(
       25) &  //  Bits 31:16 – DIV[15:0] Division Factor
              // ^^ These bits represent a division value for the corresponding
              // Generator. The actual division factor is dependent on the state of
              // DIVSEL. The number of relevant DIV bits for each Generator can be seen
              // in this table. Written bits outside of the specified range will be
              // ignored. See datasheet table 14-6
              // Bits 15:14 are reserved
     ~GCLK_GENCTRL_RUNSTDBY &  // Bit 13 – RUNSTDBY Run in Standby
     // ^^ This bit is used to keep the Generator running in Standby as long as it is
     // configured to output to a dedicated GCLK_IOn pin. If GENCTRLn.OE is zero, this
     // bit has no effect and the generator will only be running if a peripheral
     // requires the clock.
     // For SDI-12, we do *not* run in standby
     ~GCLK_GENCTRL_DIVSEL &  // Bit 12 – DIVSEL Divide Selection
     // ^^ This bit determines how the division factor of the clock source of the
     // Generator will be calculated from DIV. If the clock source should not be
     // divided, DIVSEL must be 0 and the GENCTRLn.DIV value must be either 0 or 1.
     // For SDI-12, we set this to 0 to divide by the value in the Division Factor bits
     // (ie, 512)
     ~GCLK_GENCTRL_OE) |  // Bit 11 – OE Output Enable
    // ^^ This bit is used to output the Generator clock output to the corresponding pin
    // (GCLK_IO[7..0]), as long as GCLK_IOn is not defined as the Generator source in
    // the GENCTRLn.SRC bit field.
    // For SDI-12, we don't need to enable output
    // GCLK_GENCTRL_OOV |  // Bit 10 – OOV Output Off Value
    // ^^ This bit is used to control the clock output value on pin (GCLK_IO[7..0]) when
    // the Generator is turned off or the OE bit is zero, as long as GCLK_IOn is not
    // defined as the Generator source in the GENCTRLn.SRC bit field.
    // For SDI-12, we don't need to enable output or have an output value
    (GCLK_GENCTRL_IDC |    // Bit 9 = Improve Duty Cycle
                           // ^^ This bit is used to improve the duty cycle of the
                           // Generator output to 50/50 for odd division factors. For
                           // SDI-12, set the generator output clock duty cycle to 50/50
     GCLK_GENCTRL_GENEN |  // Bit 8 Generator Enable
     // ^^ This bit is used to enable and disable the Generator.
     // Enable the generator!
     // Bits 7:4 are reserved
     GCLK_GENCTRL_SRC(
       GCLK_GENCTRL_SRC_OSCULP32K)  // Bits 3:0 Generator Clock Source Selection
     // ^^ These bits select the Generator clock source, as shown in this table. (See
     // datasheet table 14-3)
     // MAIN_CLOCK_SOURCE = GCLK_GENCTRL_SRC_DPLL0 = 120 MHz primary clock
     // GCLK_GENCTRL_SRC_OSCULP32K = 32 kHz Ultra Low Power Internal Oscillator
     // (OSCULP32K)
     // GCLK_GENCTRL_SRC_XOSC32K = 32 kHz External Crystal Oscillator (XOSC32K)
     // For SDI-12, let's use the internal oscillator, just in case the board is
     // crystalless (without an external oscillator)
    );

  while (GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_SDI12_SYNC) {
    /* Wait for synchronization */
  }

  // Enable peripheral control of the Timer Controller from the generic clock generator
  GCLK->PCHCTRL[SDI12_TC_GCLK_ID].reg =
    // Bit 7 – WRTLOCK Write Lock
    // ^^ After this bit is set to '1', further writes to the PCHCTRLm register will be
    // discarded. The control register of the corresponding Generator n (GENCTRLn), as
    // assigned in PCHCTRLm.GEN, will also be locked. It can only be unlocked by a Power
    // Reset.
    // For SDI-12, ignore this bit
    GCLK_PCHCTRL_CHEN |
    // Bit 6 – CHEN Channel Enable
    // ^^ This bit is used to enable and disable a Peripheral Channel.
    // We're enabling the channel
    // Bits 5:4 are reserved
    GENERIC_CLOCK_GENERATOR_SDI12
    // Bits 3:0 – GEN[3:0] Generator Selection
    // ^^ This bit field selects the Generator to be used as the source of a peripheral
    // clock, as shown in the table 14-7 of the datasheet
    // For SDI-12, we select generic clock generator 6
    ;

  // configure the control register for the timer control
  SDI12_TC->COUNT16.CTRLA.reg = ~TC_CTRLA_CAPTEN0 &  // Disable capture for channel 0
    ~TC_CTRLA_CAPTEN1 &                              // Disable capture for channel 1
    ~TC_CTRLA_RUNSTDBY &                             // Disable run on standby
    (TC_CTRLA_PRESCALER_DIV16 |                      // Set prescaler to 16
     TCC_CTRLA_PRESCSYNC_GCLK |  // Reload or reset the counter on next generic clock
     TC_CTRLA_MODE_COUNT16 |     // Put the timer TC3 into 16-bit mode
     TC_CTRLA_ENABLE             // Enable TC3
    );
  WAIT_TC16_REGS_SYNC(SDI12_TC)
}

// NOT resetting the SAMD timer settings
void SDI12Timer::resetSDI12TimerPrescale(void) {
  resetTC(SDI12_TC);

  // Disable generic clock generator
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_SDI12].reg =
    ~GCLK_GENCTRL_GENEN;  // Disable the generic clock control

  while (GCLK->SYNCBUSY.reg & GENERIC_CLOCK_GENERATOR_SDI12_SYNC) {
    /* Wait for synchronization */
  }
}

// Espressif ESP32/ESP8266 boards
//
#elif defined(ESP32) || defined(ESP8266)

void SDI12Timer::configSDI12TimerPrescale(void) {}

void SDI12Timer::resetSDI12TimerPrescale(void) {}

sdi12timer_t ESPFAMILY_USE_INSTRUCTION_RAM SDI12Timer::SDI12TimerRead(void) {
  return ((sdi12timer_t)(micros()));
}

// Unknown board
#else
#error "Please define your board timer and pins"
#endif
