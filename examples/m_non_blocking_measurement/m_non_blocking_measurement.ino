/**
 * @example{lineno} m_non_blocking_measurement.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Codex, at instruction of Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Example M: Advanced non-blocking measurement workflow.
 *
 * This example acknowledges and identifies one sensor, requests a CRC measurement,
 * demonstrates recovery from an undersized data buffer, and retrieves every value.
 * No recorder operation blocks the loop; update() advances the active transaction.
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

const int8_t   dataPin       = SDI12_DATA_PIN;
const int8_t   powerPin      = SDI12_POWER_PIN;
const uint32_t serialBaud    = SDI12_TERMINAL_BAUD;
const char     sensorAddress = '0';
const bool     useCRC        = true;

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

float values[9];

enum WorkflowStage : uint8_t {
  START_ACKNOWLEDGEMENT,
  WAIT_ACKNOWLEDGEMENT,
  START_IDENTIFICATION,
  WAIT_IDENTIFICATION,
  START_MEASUREMENT,
  WAIT_MEASUREMENT,
  START_CAPACITY_CHECK,
  WAIT_CAPACITY_CHECK,
  START_DATA,
  WAIT_DATA,
  WAIT_NEXT_MEASUREMENT
};

WorkflowStage stage          = START_ACKNOWLEDGEMENT;
uint32_t      measurementEnd = 0U;

void printFailure(const __FlashStringHelper* operation) {
  Serial.print(operation);
  Serial.print(F(" failed: "));
  mySDI12Recorder.printRecorderResult(Serial);
  Serial.println();
  measurementEnd = millis();
  stage          = WAIT_NEXT_MEASUREMENT;
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000UL);

  if (powerPin >= 0) {
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(10000UL);
  }

  mySDI12.begin();

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
  pinMode(dataPin, INPUT_PULLUP);
  enableInterrupt(dataPin, SDI12::handleInterrupt, CHANGE);
#endif

  Serial.println(F("Starting the non-blocking recorder workflow."));
}

void loop() {
  mySDI12Recorder.update();

  switch (stage) {
    case START_ACKNOWLEDGEMENT:
      mySDI12Recorder.startRequestAcknowledgement(sensorAddress);
      stage = WAIT_ACKNOWLEDGEMENT;
      break;

    case WAIT_ACKNOWLEDGEMENT:
      if (mySDI12Recorder.isBusy()) { break; }
      if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
        printFailure(F("Acknowledgement"));
        break;
      }
      Serial.println(F("Sensor acknowledged."));
      stage = START_IDENTIFICATION;
      break;

    case START_IDENTIFICATION:
      mySDI12Recorder.startIdentify(sensorAddress);
      stage = WAIT_IDENTIFICATION;
      break;

    case WAIT_IDENTIFICATION:
      if (mySDI12Recorder.isBusy()) { break; }
      if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
        printFailure(F("Identification"));
        break;
      }
      mySDI12Recorder.printSensorInfo(mySDI12Recorder.identification(), Serial);
      stage = START_MEASUREMENT;
      break;

    case START_MEASUREMENT:
      mySDI12Recorder.startMeasurement(sensorAddress, 0U, useCRC);
      stage = WAIT_MEASUREMENT;
      break;

    case WAIT_MEASUREMENT:
      if (mySDI12Recorder.isBusy()) { break; }
      if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
        printFailure(F("Measurement"));
        break;
      }
      Serial.print(F("Sensor advertised "));
      Serial.print(mySDI12Recorder.expectedValueCount());
      Serial.println(F(" values."));
      stage = mySDI12Recorder.expectedValueCount() == 0U ? START_DATA
                                                         : START_CAPACITY_CHECK;
      break;

    case START_CAPACITY_CHECK:
      // This local error does not transmit and leaves the measurement context intact.
      mySDI12Recorder.requestData(sensorAddress, values,
                                  mySDI12Recorder.expectedValueCount() - 1U);
      stage = WAIT_CAPACITY_CHECK;
      break;

    case WAIT_CAPACITY_CHECK:
      if (mySDI12Recorder.isBusy()) { break; }
      if (mySDI12Recorder.result() == SDI12Recorder::RESULT_DATA_BUFFER_TOO_SMALL) {
        Serial.println(F("The intentional capacity error was reported correctly."));
      } else {
        printFailure(F("Capacity check"));
        break;
      }
      stage = START_DATA;
      break;

    case START_DATA:
      mySDI12Recorder.requestData(sensorAddress, values, 9U);
      stage = WAIT_DATA;
      break;

    case WAIT_DATA:
      if (mySDI12Recorder.isBusy()) { break; }
      if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
        printFailure(F("Data retrieval"));
        break;
      }
      for (size_t i = 0U; i < mySDI12Recorder.dataValueCount(); ++i) {
        Serial.print(F("Value "));
        Serial.print(i + 1U);
        Serial.print(F(": "));
        Serial.println(values[i], 7);
      }
      measurementEnd = millis();
      stage          = WAIT_NEXT_MEASUREMENT;
      break;

    case WAIT_NEXT_MEASUREMENT:
      if (millis() - measurementEnd >= 10000UL) { stage = START_MEASUREMENT; }
      break;
  }
}
