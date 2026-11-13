/**
 * @file SDI12EnableInterruptFallback.cpp
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Provides the default EnableInterrupt implementation for AVR builds.
 */


#include "SDI12Config.h"

#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && !defined(SDI12_INTERNAL_PCINT)
#include <Arduino.h>

void EI_SerialPrint_P(const char* str) __attribute__((weak));
void bogusFunctionPlaceholder(void) __attribute__((weak));
void disableInterruptFallback(uint8_t interruptDesignator);
void enableInterruptFallback(uint8_t interruptDesignator, void (*userFunction)(void),
                             uint8_t mode);

// Make the original externally visible EnableInterrupt symbols weak. This keeps the
// fallback available to legacy sketches while allowing a strong implementation from
// any sketch or required library object to replace it regardless of PlatformIO archive
// order.
#pragma weak arduinoInterruptedPin
#pragma weak arduinoPinState
#pragma weak externalFunctionPointer
#pragma weak fallingPinsPORTA
#pragma weak fallingPinsPORTB
#pragma weak fallingPinsPORTC
#pragma weak fallingPinsPORTD
#pragma weak fallingPinsPORTJ
#pragma weak fallingPinsPORTK
#pragma weak functionPointerArrayEXTERNAL
#pragma weak pcmsk
#pragma weak portAFunctions
#pragma weak portBFunctions
#pragma weak portCFunctions
#pragma weak portDFunctions
#pragma weak portJFunctions
#pragma weak portJPCMSK
#pragma weak portKFunctions
#pragma weak risingPinsPORTA
#pragma weak risingPinsPORTB
#pragma weak risingPinsPORTC
#pragma weak risingPinsPORTD
#pragma weak risingPinsPORTJ
#pragma weak risingPinsPORTK
#pragma weak __vector_1
#pragma weak __vector_2
#pragma weak __vector_3
#pragma weak __vector_4
#pragma weak __vector_5
#pragma weak __vector_6
#pragma weak __vector_7
#pragma weak __vector_8
#pragma weak __vector_9
#pragma weak __vector_10
#pragma weak __vector_11
#define disableInterrupt disableInterruptFallback
#define enableInterrupt enableInterruptFallback
#include <EnableInterrupt.h>
#undef disableInterrupt
#undef enableInterrupt

void disableInterrupt(uint8_t interruptDesignator) __attribute__((weak));
void enableInterrupt(uint8_t interruptDesignator, void (*userFunction)(void),
                     uint8_t mode) __attribute__((weak));

void enableInterrupt(uint8_t interruptDesignator, void (*userFunction)(void),
                     uint8_t mode) {
#if defined(digitalPinToInterrupt) && defined(NOT_AN_INTERRUPT)
  if (!(interruptDesignator & PINCHANGEINTERRUPT)) {
    const uint8_t interruptNumber = digitalPinToInterrupt(interruptDesignator);
    if (interruptNumber != static_cast<uint8_t>(NOT_AN_INTERRUPT)) {
      attachInterrupt(interruptNumber, userFunction, mode);
      return;
    }
  }
#endif
  enableInterruptFallback(interruptDesignator, userFunction, mode);
}

void disableInterrupt(uint8_t interruptDesignator) {
#if defined(digitalPinToInterrupt) && defined(NOT_AN_INTERRUPT)
  if (!(interruptDesignator & PINCHANGEINTERRUPT)) {
    const uint8_t interruptNumber = digitalPinToInterrupt(interruptDesignator);
    if (interruptNumber != static_cast<uint8_t>(NOT_AN_INTERRUPT)) {
      detachInterrupt(interruptNumber);
      return;
    }
  }
#endif
  disableInterruptFallback(interruptDesignator);
}
#endif
