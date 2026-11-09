/**
 * @example{lineno} raw_extended_commands.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Codex, at instruction of Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Send a manufacturer-specific extended command through SDI12Recorder.
 */

#include <SDI12Config.h>
#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && !defined(SDI12_INTERNAL_PCINT)
#include <EnableInterrupt.h>
#endif

#include <SDI12Recorder.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif
#ifndef SDI12_TERMINAL_BAUD
#define SDI12_TERMINAL_BAUD 115200
#endif

const int8_t   dataPin       = SDI12_DATA_PIN;
const int8_t   powerPin      = SDI12_POWER_PIN;
const uint32_t serialBaud    = SDI12_TERMINAL_BAUD;
const char     sensorAddress = '0';

// Replace X with the command body documented by the sensor manufacturer. Do not
// include the sensor address or terminating exclamation point.
const String extendedCommand = "X";

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

bool commandStarted = false;
bool resultPrinted  = false;

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000UL);

  if (powerPin >= 0) {
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(10000UL);
  }

  mySDI12.begin();
}

void loop() {
  if (!commandStarted) {
    mySDI12Recorder.startRawCommand(sensorAddress, extendedCommand);
    commandStarted = true;
  }

  mySDI12Recorder.update();

  if (!resultPrinted && mySDI12Recorder.isOperationComplete()) {
    if (mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS) {
      Serial.print(F("Opaque response payload: "));
      Serial.println(mySDI12Recorder.response());
    } else {
      Serial.print(F("Extended command failed: "));
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
    }
    resultPrinted = true;
  }
}
