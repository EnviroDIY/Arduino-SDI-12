/**
 * @example{lineno} TestWarmUp.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 * @date October 2026
 *
 * @brief Measures a sensor's power-on warm-up time.
 *
 * This sketch measures from the moment sensor power is enabled until the sensor first
 * returns a valid acknowledgement and then valid identification information. It uses
 * SDI12Recorder so each operation receives the protocol-compliant retry behavior used
 * by normal recorder applications.
 */

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
#include <EnableInterrupt.h>
#endif

#include <SDI12.h>
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
#ifndef SDI12_SENSOR_ADDRESS
#define SDI12_SENSOR_ADDRESS '0'
#endif
#ifndef SDI12_POWER_OFF_MS
#define SDI12_POWER_OFF_MS 60000UL
#endif
#ifndef SDI12_MAX_WARM_UP_MS
#define SDI12_MAX_WARM_UP_MS 180000UL
#endif
#ifndef SDI12_WARM_UP_TESTS
#define SDI12_WARM_UP_TESTS 5U
#endif

const uint32_t serialBaud    = SDI12_TERMINAL_BAUD;
const int8_t   dataPin       = SDI12_DATA_PIN;
const int8_t   powerPin      = SDI12_POWER_PIN;
const char     sensorAddress = SDI12_SENSOR_ADDRESS;

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

bool runWarmUpTest(uint8_t testNumber, uint32_t& acknowledgementMs,
                   uint32_t& identificationMs) {
  acknowledgementMs = static_cast<uint32_t>(-1);
  identificationMs  = static_cast<uint32_t>(-1);

  Serial.println();
  Serial.println(F("------------------------------------------------------------"));
  Serial.print(F("Warm-up test "));
  Serial.print(testNumber);
  Serial.print(F(" of "));
  Serial.println(SDI12_WARM_UP_TESTS);

  Serial.print(F("Powering sensor off for "));
  Serial.print(SDI12_POWER_OFF_MS);
  Serial.println(F(" ms."));
  digitalWrite(powerPin, LOW);
  delay(SDI12_POWER_OFF_MS);
  mySDI12.clearBuffer();

  Serial.println(F("Powering sensor on and beginning acknowledgement attempts."));
  const uint32_t powerOnTime = millis();
  digitalWrite(powerPin, HIGH);

  while (millis() - powerOnTime <= SDI12_MAX_WARM_UP_MS) {
    if (mySDI12Recorder.requestAcknowledgement(sensorAddress)) {
      acknowledgementMs = millis() - powerOnTime;
      Serial.print(F("First valid acknowledgement after "));
      Serial.print(acknowledgementMs);
      Serial.println(F(" ms."));
      break;
    }

    Serial.print(F("Acknowledgement attempt ended after "));
    Serial.print(millis() - powerOnTime);
    Serial.print(F(" ms: "));
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }

  if (acknowledgementMs == static_cast<uint32_t>(-1)) {
    Serial.println(F("No valid acknowledgement before the warm-up test limit."));
    return false;
  }

  SDI12Recorder::Identification identification;
  while (millis() - powerOnTime <= SDI12_MAX_WARM_UP_MS) {
    if (mySDI12Recorder.identify(sensorAddress, identification)) {
      identificationMs = millis() - powerOnTime;
      Serial.print(F("First valid identification after "));
      Serial.print(identificationMs);
      Serial.println(F(" ms."));
      mySDI12Recorder.printSensorInfo(identification, Serial);
      return true;
    }

    Serial.print(F("Identification attempt ended after "));
    Serial.print(millis() - powerOnTime);
    Serial.print(F(" ms: "));
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }

  Serial.println(F("No valid identification before the warm-up test limit."));
  return false;
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000UL) {}

  Serial.print(F("Opening SDI-12 bus on pin "));
  Serial.print(dataPin);
  Serial.println(F("..."));
  mySDI12.begin();

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
  pinMode(dataPin, INPUT_PULLUP);
  enableInterrupt(dataPin, SDI12::handleInterrupt, CHANGE);
#endif

  if (powerPin < 0) {
    Serial.println(F("This test requires a controllable sensor power pin."));
    while (true) {}
  }
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, LOW);
}

void loop() {
  uint32_t acknowledgementTotal = 0U;
  uint32_t identificationTotal  = 0U;
  uint32_t acknowledgementMax   = 0U;
  uint32_t identificationMax    = 0U;
  uint8_t  successfulTests      = 0U;

  for (uint8_t test = 1U; test <= SDI12_WARM_UP_TESTS; ++test) {
    uint32_t acknowledgementMs = 0U;
    uint32_t identificationMs  = 0U;
    if (!runWarmUpTest(test, acknowledgementMs, identificationMs)) { continue; }

    acknowledgementTotal += acknowledgementMs;
    identificationTotal += identificationMs;
    if (acknowledgementMs > acknowledgementMax) {
      acknowledgementMax = acknowledgementMs;
    }
    if (identificationMs > identificationMax) { identificationMax = identificationMs; }
    ++successfulTests;
  }

  digitalWrite(powerPin, LOW);
  Serial.println();
  Serial.println(F("================ Warm-up summary ================"));
  Serial.print(F("Successful tests: "));
  Serial.print(successfulTests);
  Serial.print(F(" of "));
  Serial.println(SDI12_WARM_UP_TESTS);
  if (successfulTests > 0U) {
    Serial.print(F("Mean acknowledgement time: "));
    Serial.print(acknowledgementTotal / successfulTests);
    Serial.println(F(" ms"));
    Serial.print(F("Maximum acknowledgement time: "));
    Serial.print(acknowledgementMax);
    Serial.println(F(" ms"));
    Serial.print(F("Mean identification time: "));
    Serial.print(identificationTotal / successfulTests);
    Serial.println(F(" ms"));
    Serial.print(F("Maximum identification time: "));
    Serial.print(identificationMax);
    Serial.println(F(" ms"));
    Serial.print(F("Candidate _warmUpTime_ms before adding a safety margin: "));
    Serial.println(identificationMax);
  }
  Serial.println(F("Repeat with the power-off duration used by the deployment."));
  Serial.println(F("FINISHED"));
  while (true) {}
}
