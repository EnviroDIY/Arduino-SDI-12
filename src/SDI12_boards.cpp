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
uint16_t SDI12Timer::bitTimes(sdi12timer_t dt) {
  // multiply the time delta in ticks by the bits per tick
  return mul8x8to16(dt + RX_WINDOW_FUDGE, BITS_PER_TICK_Q10) >> 10;
}

// But nothing fancy for bigger timers
#elif TIMER_INT_SIZE == 16 || TIMER_INT_SIZE == 32
uint16_t SDI12Timer::bitTimes(sdi12timer_t dt) {
  // divide the number of ticks by the ticks per bit
  return static_cast<uint16_t>((dt + static_cast<sdi12timer_t>(RX_WINDOW_FUDGE)) /
                               static_cast<sdi12timer_t>(TICKS_PER_BIT));
}
#else
#error "Board timer is incorrectly configured!"
#endif


// Most 'standard' AVR boards
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

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT2;
}

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR2A = TCCR2A;
  preSDI12_TCCR2B = TCCR2B;
#if F_CPU == 16000000L
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on -
                  // prescaler set to CK/1024
#elif F_CPU == 12000000L
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x07;  // TCCR2B = 0x07 = 0b00000111 - Clock Select bits 22, 21, & 20 on -
                  // prescaler set to CK/1024
#elif F_CPU == 8000000L
  TCCR2A = 0x00;  // TCCR2A = 0x00 = "normal" operation - Normal port operation, OC2A &
                  // OC2B disconnected
  TCCR2B = 0x06;  // TCCR2B = 0x06 = 0b00000110 - Clock Select bits 22 & 20 on -
                  // prescaler set to CK/256
#endif
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR2A = preSDI12_TCCR2A;
  TCCR2B = preSDI12_TCCR2B;
}

// ATtiny boards (ie, adafruit trinket)
#elif defined(__AVR_ATtiny25__) | defined(__AVR_ATtiny45__) | defined(__AVR_ATtiny85__)

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT1;
}

/**
 * @brief The value of timer control register 1A prior to being set for SDI-12.
 */
static uint8_t preSDI12_TCCR1A;

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR1A = TCCR1;
#if F_CPU == 16000000L
  TCCR1           = 0b00001011;  // Set the prescaler to 1024
#elif F_CPU == 8000000L
  TCCR1 = 0b00001010;  // Set the prescaler to 512
#endif
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  TCCR1 = preSDI12_TCCR1A;
}

// Arduino Leonardo & Yun and other 32U4 boards
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || \
  defined(__AVR_ATmega32U4__)

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return TCNT4;
}

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

void SDI12Timer::configSDI12TimerPrescale(void) {
  preSDI12_TCCR4A = TCCR4A;
  preSDI12_TCCR4B = TCCR4B;
  preSDI12_TCCR4C = TCCR4C;
  preSDI12_TCCR4D = TCCR4D;
  preSDI12_TCCR4E = TCCR4E;
  TCCR4A = 0x00;  // TCCR4A = 0x00 = "normal" operation - Normal port operation, OC4A &
                  // OC4B disconnected
#if F_CPU == 16000000L
  TCCR4B = 0x0B;  // TCCR4B = 0x0B = 0b00001011 - Clock Select bits 43, 41, & 40 on -
                  // prescaler set to CK/1024
#elif F_CPU == 8000000L
  TCCR4B = 0x0A;  // TCCR4B = 0x0A = 0b00001010 - Clock Select bits 43 & 41 on -
                  // prescaler set to CK/512
#endif
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

// Arduino Zero other SAMD21 boards
#elif defined(ARDUINO_SAMD_ZERO) || defined(__SAMD21G18A__) || \
  defined(__SAMD21J18A__) || defined(__SAMD21E18A__)

/// Fully reset the TC to factory settings and disable it
static inline void resetTC(Tc* TCx) {
  // Disable TCx
  TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TCx->COUNT16.STATUS.bit.SYNCBUSY)
    ;

  // Reset TCx
  TCx->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TCx->COUNT16.STATUS.bit.SYNCBUSY)
    ;
  while (TCx->COUNT16.CTRLA.bit.SWRST)
    ;
}

/**
 * @brief The value of generic clock generator divider register prior to being set for
 * SDI-12.
 *
 * This is an 32-bit register located at 0x40000C00 + 0x8
 */
static uint32_t preSDI12_REG_GCLK_GENDIV;
/**
 * @brief The value of generic clock _generator_ control register prior to being set for
 * SDI-12.
 *
 * This is an 32-bit register located at 0x40000C00 + 0x4
 */
static uint32_t preSDI12_REG_GCLK_GENCTRL;
/**
 * @brief The value of generic clock control register prior to being set for
 * SDI-12.
 *
 * This is an 32-bit register located at 0x40000C00 + 0x2
 */
static uint8_t preSDI12_REG_GCLK_CLKCTRL;

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  return REG_TC3_COUNT8_COUNT;
}

void SDI12Timer::configSDI12TimerPrescale(void) {
  // read control register values prior to changes
  preSDI12_REG_GCLK_GENDIV  = REG_GCLK_GENDIV;
  preSDI12_REG_GCLK_GENCTRL = REG_GCLK_GENCTRL;
  preSDI12_REG_GCLK_CLKCTRL = REG_GCLK_CLKCTRL;

  // Set up the generic clock generator divisor register
  // NOTE: Could write the below as GCLK->GENDIV.reg instead of REG_GCLK_GENDIV
  REG_GCLK_GENDIV = GCLK_GENDIV_ID(4) |  // Select Generic Clock Generator 4
    GCLK_GENDIV_DIV(6);                  // Divide the clock source by divisor 6
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  ;  // Wait for synchronization


  // Set up the generic clock generator control register
  // NOTE: Could write the below as GCLK->GENCTRL.reg instead ofREG_GCLK_GENCTRL
  REG_GCLK_GENCTRL = (GCLK_GENCTRL_ID(4) |        // Select GCLK4
                      GCLK_GENCTRL_SRC_DFLL48M |  // Select the 48MHz clock source
                      GCLK_GENCTRL_IDC |     // Set the duty cycle to 50/50 HIGH/LOW
                      GCLK_GENCTRL_GENEN) &  // Enable the generic clock clontrol
    ~GCLK_GENCTRL_RUNSTDBY &                 // Do NOT run in stand by
    ~GCLK_GENCTRL_DIVSEL;  // Divide clock source by GENDIV.DIV: 48MHz/5=9.6MHz
                           // ^^ & ~ for DIVSEL to set DIVSEL to 0
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  ;  // Wait for synchronization

  // Set up the generic clock control register
  // NOTE: Could write the below as GCLK->CLKCTRL.reg instead of REG_GCLK_CLKCTRL
  // Feed GCLK4 to TC3 (also feeds to TCC2, the two must have the same source).
  // TC3 (and TCC2) seem to be free, so I'm using them.
  // TC4 is used by Tone and Servo, TC5 is tied to the same clock as TC4.
  // TC6 and TC7 are not available on all boards.
  REG_GCLK_CLKCTRL = GCLK_CLKCTRL_GEN_GCLK4 |  // Select Generic Clock Generator
                                               // 4
    GCLK_CLKCTRL_CLKEN |                       // Enable the generic clock generator
    GCLK_CLKCTRL_ID_TCC2_TC3;  // Feed the Generic Clock Generator 4 to TCC2 and TC3

  while (GCLK->STATUS.bit.SYNCBUSY)
    ;
  ;  // Wait for synchronization

  // fully software reset and disable the TC before we start messing with it
  resetTC(SDI12_TC);

  // Set up the control register for Timer Controller 3
  REG_TC3_CTRLA |=
    TC_CTRLA_PRESCALER_DIV16 |  // Set prescaler to 16, 9.6MHz/16 = 600kHz
    TC_CTRLA_WAVEGEN_NFRQ |     // Put the timer TC3 into normal frequency (NFRQ) mode
    TC_CTRLA_MODE_COUNT16 |     // Put the timer TC3 into 16-bit mode
    TC_CTRLA_ENABLE;            // Enable TC3
  while (SDI12_TC->COUNT16.STATUS.bit.SYNCBUSY)
    ;  // Wait for synchronization
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  // reset the generic clock generator divisor register
  REG_GCLK_GENDIV = preSDI12_REG_GCLK_GENDIV;
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;  // Wait for synchronization

  // reset the generic clock generator control register
  REG_GCLK_GENCTRL = preSDI12_REG_GCLK_GENCTRL;
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;  // Wait for synchronization

  // reset the generic clock control register
  REG_GCLK_CLKCTRL = preSDI12_REG_GCLK_CLKCTRL;
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;  // Wait for synchronization

  // fully software reset the control register for Timer Controller 3 and then disable
  // it
  resetTC(SDI12_TC);
}

// SAMD51 and SAME51 boards
#elif defined(__SAMD51__) || defined(__SAME51__)

/// Fully reset the TC to factory settings and disable it
static inline void resetTC(Tc* TCx) {
  // Disable TCx
  TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;  // unset enable bit
  while (TCx->COUNT16.SYNCBUSY.bit.ENABLE)
    ;  // wait for enable sync busy bit to clear

  // Reset TCx with SWRST (Software Reset) bit
  // - Writing a '0' to this bit has no effect.
  // - Writing a '1' to this bit resets all registers in the TC, except DBGCTRL, to
  //   their initial state, and the TC will be disabled.
  // - Writing a '1' to CTRLA.SWRST will always take precedence; all other writes in the
  //   same write-operation will be discarded.

  TCx->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  while (TCx->COUNT16.SYNCBUSY.bit.SWRST)
    ;  // wait for software reset busy bit to clear
}

/**
 * @brief The value of generic clock _generator_ control register prior to being set for
 * SDI-12.
 *
 * This is an 32-bit register offset from the GCLK register by 0x20 + n*0x04 [n=0..11],
 * where n is the generic clock number.
 */
static uint32_t preSDI12_REG_GCLK_GENCTRL;
/**
 * @brief The value of generic clock peripheral control channel register prior to being
 * set for SDI-12.
 *
 * This is an 32-bit register offset from the GCLK register by 0x80 + m*0x04 [m=0..47],
 * where m is the peripheral channel number.
 *
 * @see docs/SAMD51PeripheralClocks.dox for a list of the peripheral channel numbers.
 */
static uint32_t preSDI12_REG_GCLK_PCHCTRL;

sdi12timer_t SDI12Timer::SDI12TimerRead(void) {
  // Note from datasheet: Prior to any read access, this register must be synchronized
  // by user by writing the according TC Command value to the Control B Set register
  // (CTRLBSET.CMD=READSYNC)
  // see:
  // https://onlinedocs.microchip.com/oxy/GUID-F5813793-E016-46F5-A9E2-718D8BCED496-en-US-13/GUID-5033DFD7-EB2D-4870-AE98-D40CADB0531E.html

  // Code taken from Microchip article on how to read the tiemr value
  // https://microchip.my.site.com/s/article/SAM-D5x-E5x--Reading-TC-TCC-COUNT-register


  // write READSYNC command to the Control B Set register
  SDI12_TC->COUNT16.CTRLBSET.reg = TC_CTRLBSET_CMD_READSYNC;

  // wait for the CMD bits in CTRLBSET to be cleared, meaning the CMD has been executed
  while (SDI12_TC->COUNT16.CTRLBSET.reg & TC_CTRLBSET_CMD_READSYNC)
    ;

  // read the COUNT register
  return SDI12_TC->COUNT16.COUNT.reg;
}

void SDI12Timer::configSDI12TimerPrescale(void) {
  // read the values of the registers prior to making changes
  preSDI12_REG_GCLK_GENCTRL = GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_SDI12].reg;
  preSDI12_REG_GCLK_PCHCTRL = GCLK->PCHCTRL[SDI12_TC_GCLK_ID].reg;

  // Calculate the new value for the generic clock _generator_ control register
  uint32_t postSDI12_REG_GCLK_GENCTRL =
    (GCLK_GENCTRL_DIV(
       15) &  //  Bits 31:16 – DIV[15:0] Division Factor
              // ^^ These bits represent a division value for the corresponding
              // Generator. The actual division factor is dependent on the state of
              // DIVSEL. The number of relevant DIV bits for each Generator can be seen
              // in this table. Written bits outside of the specified range will be
              // ignored. See datasheet table 14-6
              // ((0xFFFFU << 16) & ((15) << 16)) = 0b00000000000011110000000000000000
              // Bits 15:14 are reserved
     ~GCLK_GENCTRL_RUNSTDBY &  // Bit 13 – RUNSTDBY Run in Standby
     // ^^ This bit is used to keep the Generator running in Standby as long as it is
     // configured to output to a dedicated GCLK_IOn pin. If GENCTRLn.OE is zero, this
     // bit has no effect and the generator will only be running if a peripheral
     // requires the clock.
     // (0x1U << 13) = ~0b00000000000000000010000000000000 =
     //                 0b11111111111111111101111111111111
     // For SDI-12, we do *not* run in standby
     ~GCLK_GENCTRL_DIVSEL &  // Bit 12 – DIVSEL Divide Selection
     // ^^ This bit determines how the division factor of the clock source of the
     // Generator will be calculated from DIV. If the clock source should not be
     // divided, DIVSEL must be 0 and the GENCTRLn.DIV value must be either 0 or 1.
     // (0x1U << 12) = ~0b00000000000000000001000000000000
     //                 0b11111111111111111110111111111111
     // For SDI-12, we set this to 0 to divide by the value in the Division Factor bits
     // (ie, 512)
     ~GCLK_GENCTRL_OE) |  // Bit 11 – OE Output Enable
    // ^^ This bit is used to output the Generator clock output to the corresponding pin
    // (GCLK_IO[7..0]), as long as GCLK_IOn is not defined as the Generator source in
    // the GENCTRLn.SRC bit field.
    // (0x1U << 11) = ~0b00000000000000000000100000000000
    //                 0b11111111111111111111011111111111
    // For SDI-12, we don't need to enable output
    // GCLK_GENCTRL_OOV |  // Bit 10 – OOV Output Off Value
    // ^^ This bit is used to control the clock output value on pin (GCLK_IO[7..0]) when
    // the Generator is turned off or the OE bit is zero, as long as GCLK_IOn is not
    // defined as the Generator source in the GENCTRLn.SRC bit field.
    // (0x1U << 10) = ~0b00000000000000000000010000000000
    //                 0b11111111111111111111101111111111
    // For SDI-12, we don't need to enable output or have an output value
    (GCLK_GENCTRL_IDC |    // Bit 9 = Improve Duty Cycle
                           // ^^ This bit is used to improve the duty cycle of the
                           // Generator output to 50/50 for odd division factors. For
                           // SDI-12, set the generator output clock duty cycle to 50/50
                           // (0x1U << 9) = 0b00000000000000000000001000000000
     GCLK_GENCTRL_GENEN |  // Bit 8 Generator Enable
     // ^^ This bit is used to enable and disable the Generator.
     // (0x1U << 8) = ~0b00000000000000000000000100000000
     // Enable the generator!
     // Bits 7:4 are reserved
     GCLK_GENCTRL_SRC(
       GCLK_GENCTRL_SRC_DPLL0)  // Bits 3:0 Generator Clock Source Selection
     // ^^ These bits select the Generator clock source, as shown in this table. (See
     // datasheet table 14-3)
     // MAIN_CLOCK_SOURCE = GCLK_GENCTRL_SRC_DPLL0 = 120 MHz primary clock
     // (0x7U << 0) = 0b00000000000000000000000000000111
     // GCLK_GENCTRL_SRC_OSCULP32K = 32 kHz Ultra Low Power Internal Oscillator
     // (OSCULP32K)
     // (0x3U << 0) = 0b00000000000000000000000000000011
     // GCLK_GENCTRL_SRC_XOSC32K = 32 kHz External Crystal Oscillator (XOSC32K)
     // (0x5U << 0) = 0b00000000000000000000000000000101
    );

  // Set the generator control register for the clock generator for the selected clock
  // generator source
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_SDI12].reg = postSDI12_REG_GCLK_GENCTRL;
  while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SDI12)
    ;  // Wait for the SDI-12 clock generator sync busy bit to clear

  // Calculate the new value for the generic clock peripheral control channel register.
  uint8_t postSDI12_REG_GCLK_PCHCTRL =
    // Bit 7 – WRTLOCK Write Lock
    // ^^ After this bit is set to '1', further writes to the PCHCTRLm register will
    // be discarded. The control register of the corresponding Generator n (GENCTRLn),
    // as assigned in PCHCTRLm.GEN, will also be locked. It can only be unlocked by a
    // Power Reset. For SDI-12, ignore this bit
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

  // Set the generic clock peripheral control channel register
  GCLK->PCHCTRL[SDI12_TC_GCLK_ID].reg = postSDI12_REG_GCLK_PCHCTRL;
  while (!GCLK->PCHCTRL[SDI12_TC_GCLK_ID].bit.CHEN)
    ;  // wait to finish enabling

  // fully software reset and disable the TC before we start messing with it
  resetTC(SDI12_TC);

  // Set timer counter mode as normal PWM
  SDI12_TC->COUNT16.WAVE.bit.WAVEGEN = TCC_WAVE_WAVEGEN_NPWM_Val;
  // NOTE: This register isn't synced, so no wait needed

  // Set the timer to count up
  // >> This bit is used to change the direction of the counter.
  // >> Writing a '0' to this bit has no effect
  // >> Writing a '1' to this bit will clear the bit and make the counter count up.
  // SRGD Note: Writing to 1 actually flips the direction from whatever it was, not
  // reset it to up
  if (SDI12_TC->COUNT16.CTRLBSET.bit
        .DIR) {  // check the current direction first (0=counting up, 1=counting down)
    SDI12_TC->COUNT16.CTRLBSET.bit.DIR = 1;
    while (SDI12_TC->COUNT16.SYNCBUSY.bit.CTRLB)
      ;  // wait the control B sync busy bit to clear
  }

  // configure the control register for the timer control
  uint32_t postSDI12_REG_TC_CTRLA =
    ~TC_CTRLA_CAPTEN0 &          // Disable capture for channel 0 (use for compare)
    ~TC_CTRLA_CAPTEN1 &          // Disable capture for channel 1 (use for compare)
    ~TC_CTRLA_RUNSTDBY &         // Disable run on standby
    (TC_CTRLA_PRESCALER_DIV16 |  // Set prescaler to 16
     TCC_CTRLA_PRESCSYNC_GCLK |  // Reload or reset the counter on next generic clock
     TC_CTRLA_MODE_COUNT16 |     // Put the timer TC3 into 16-bit mode
     TC_CTRLA_ENABLE             // Enable TC3
    );

  SDI12_TC->COUNT16.CTRLA.reg = postSDI12_REG_TC_CTRLA;
  while (SDI12_TC->COUNT16.SYNCBUSY.bit.ENABLE)
    ;  // wait for to finish enabling
}

void SDI12Timer::resetSDI12TimerPrescale(void) {
  // Reset the generator control register for the clock generator
  GCLK->GENCTRL[GENERIC_CLOCK_GENERATOR_SDI12].reg = preSDI12_REG_GCLK_GENCTRL;
  while (GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SDI12)
    ;  // Wait for the SDI-12 clock generator sync busy bit to clear

  // Reset the generic clock peripheral control channel register
  GCLK->PCHCTRL[SDI12_TC_GCLK_ID].reg = preSDI12_REG_GCLK_PCHCTRL;
  while (!GCLK->PCHCTRL[SDI12_TC_GCLK_ID].bit.CHEN)
    ;  // wait to finish enabling ??

  // fully software reset the control register for SDI-12 Timer Controller and then
  // disable it
  resetTC(SDI12_TC);
}

// Espressif ESP32/ESP8266 boards or other boards faster than 48MHz
// WARNING: I haven't tested the minimum speed that this will work at!
#elif defined(ESP32) || defined(ESP8266) || F_CPU >= 48000000L

void SDI12Timer::configSDI12TimerPrescale(void) {}

void SDI12Timer::resetSDI12TimerPrescale(void) {}

sdi12timer_t ISR_MEM_ACCESS SDI12Timer::SDI12TimerRead(void) {
  return (static_cast<sdi12timer_t>(micros()));
}

// Unknown board
#else
#error "Please define your board timer and pins"
#endif
