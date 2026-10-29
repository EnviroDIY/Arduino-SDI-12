/**
 * @example{lineno} a_wild_card.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example A: Using the Wildcard - Getting Single Sensor Information
 *
 * This is a simple demonstration of the SDI-12 library for Arduino.
 *
 * It requests information about the attached sensor, including its address and
 * manufacturer info.
 */

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
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

uint32_t serialBaud = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   dataPin    = SDI12_DATA_PIN;      /*!< The pin of the SDI-12 data bus */
int8_t   powerPin   = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */

/** Define the SDI-12 bus and recorder */
SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.print("Opening SDI-12 bus on pin ");
  Serial.print(dataPin);
  Serial.println("...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  // Power the sensors;
  if (powerPin >= 0) {
    Serial.println("Powering up sensors...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(200);
  }

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
  // Enable interrupts for the receive pin
  pinMode(dataPin, INPUT_PULLUP);
  enableInterrupt(dataPin, SDI12::handleInterrupt, CHANGE);
#endif
}

void loop() {
  // The wildcard query works only when exactly one sensor is attached to the bus.
  const char address = mySDI12Recorder.queryAddress();
  if (address == '\0') {
    // A return of null for the address means nothing was found
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  } else {
    SDI12Recorder::Identification sensorInfo;
    if (mySDI12Recorder.identify(address, sensorInfo)) {
      mySDI12Recorder.printSensorInfo(sensorInfo, Serial);
    } else {
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
    }
  }
  delay(3000);  // print again in three seconds
}
