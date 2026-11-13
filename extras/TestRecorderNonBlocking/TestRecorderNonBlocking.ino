/**
 * @example{lineno} TestRecorderNonBlocking.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Compile and hardware exercise sketch for every public non-blocking recorder
 * operation.
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

const int8_t   dataPin        = SDI12_DATA_PIN;
const int8_t   powerPin       = SDI12_POWER_PIN;
const uint32_t serialBaud     = SDI12_TERMINAL_BAUD;
const char     targetAddress  = '0';
const char     changedAddress = '1';

// These tests can change or interrupt sensor state, so enable them deliberately.
const bool testAddressChange = false;
const bool testImmediateData = false;
const bool testCancellation  = false;
const bool testRawCommand    = false;

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

float dataValues[9];
char  copiedResponse[SDI12_RECORDER_RESPONSE_BUFFER_SIZE];
char  sensorAddress = targetAddress;

enum TestStage : uint8_t {
  START_QUERY,
  WAIT_QUERY,
  START_ACKNOWLEDGEMENT,
  WAIT_ACKNOWLEDGEMENT,
  START_IDENTIFICATION,
  WAIT_IDENTIFICATION,
  START_ADDRESS_CHANGE,
  WAIT_ADDRESS_CHANGE,
  START_MEASUREMENT,
  WAIT_MEASUREMENT,
  START_DATA,
  WAIT_DATA,
  START_CONCURRENT_MEASUREMENT,
  WAIT_CONCURRENT_MEASUREMENT,
  WAIT_CONCURRENT_READY,
  WAIT_CONCURRENT_DATA,
  START_CONTINUOUS_DATA,
  WAIT_CONTINUOUS_DATA,
  START_RAW_COMMAND,
  WAIT_RAW_COMMAND,
  TEST_COMPLETE
};

TestStage testStage = START_QUERY;

void printResult(const __FlashStringHelper* label) {
  Serial.print(label);
  Serial.print(F(": "));
  mySDI12Recorder.printRecorderResult(Serial);
  Serial.println();
}

void printDataValues() {
  for (size_t i = 0U; i < mySDI12Recorder.dataValueCount(); ++i) {
    Serial.print(F("  "));
    Serial.println(dataValues[i], 7);
  }
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
}

void loop() {
  mySDI12Recorder.update();

  switch (testStage) {
    case START_QUERY:
      mySDI12Recorder.startQueryAddress();
      testStage = WAIT_QUERY;
      break;

    case WAIT_QUERY:
      if (!mySDI12Recorder.isOperationComplete()) { break; }
      printResult(F("Address query"));
      if (mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS) {
        sensorAddress = mySDI12Recorder.queriedAddress();
      }
      testStage = START_ACKNOWLEDGEMENT;
      break;

    case START_ACKNOWLEDGEMENT:
      mySDI12Recorder.startRequestAcknowledgement(sensorAddress);
      testStage = WAIT_ACKNOWLEDGEMENT;
      break;

    case WAIT_ACKNOWLEDGEMENT:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Acknowledgement"));
      Serial.print(F("Acknowledged address: "));
      Serial.println(mySDI12Recorder.acknowledgedAddress());
      testStage = START_IDENTIFICATION;
      break;

    case START_IDENTIFICATION:
      mySDI12Recorder.startIdentify(sensorAddress);
      testStage = WAIT_IDENTIFICATION;
      break;

    case WAIT_IDENTIFICATION:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Identification"));
      if (mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS) {
        mySDI12Recorder.printSensorInfo(mySDI12Recorder.identification(), Serial);
      }
      testStage = START_ADDRESS_CHANGE;
      break;

    case START_ADDRESS_CHANGE:
      if (testAddressChange) {
        mySDI12Recorder.startChangeAddress(sensorAddress, changedAddress);
        testStage = WAIT_ADDRESS_CHANGE;
      } else {
        testStage = START_MEASUREMENT;
      }
      break;

    case WAIT_ADDRESS_CHANGE:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Address change"));
      if (mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS) {
        sensorAddress = changedAddress;
      }
      testStage = START_MEASUREMENT;
      break;

    case START_MEASUREMENT:
      mySDI12Recorder.startMeasurement(sensorAddress, 0U, true);
      testStage = WAIT_MEASUREMENT;
      break;

    case WAIT_MEASUREMENT:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Normal CRC measurement"));
      Serial.print(F("Acknowledgement millis: "));
      Serial.println(mySDI12Recorder.measurementAcknowledgementMillis());
      Serial.print(F("Advertised seconds: "));
      Serial.println(mySDI12Recorder.measurementSeconds());
      Serial.print(F("Expected values: "));
      Serial.println(mySDI12Recorder.expectedValueCount());
      Serial.print(F("Service request received: "));
      Serial.println(mySDI12Recorder.measurementServiceRequestReceived());
      Serial.print(F("Service request millis: "));
      Serial.println(mySDI12Recorder.measurementServiceRequestMillis());
      testStage = START_DATA;
      break;

    case START_DATA:
      mySDI12Recorder.requestData(sensorAddress, dataValues, 9U);
      testStage = WAIT_DATA;
      break;

    case WAIT_DATA:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Normal measurement data"));
      printDataValues();
      testStage = START_CONCURRENT_MEASUREMENT;
      break;

    case START_CONCURRENT_MEASUREMENT:
      mySDI12Recorder.startConcurrentMeasurement(sensorAddress, 0U, false);
      testStage = WAIT_CONCURRENT_MEASUREMENT;
      break;

    case WAIT_CONCURRENT_MEASUREMENT:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Concurrent measurement"));
      if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
        testStage = START_CONTINUOUS_DATA;
        break;
      }
      if (testImmediateData) {
        mySDI12Recorder.requestDataImmediately(sensorAddress, dataValues, 9U);
        testStage = WAIT_CONCURRENT_DATA;
      } else {
        testStage = WAIT_CONCURRENT_READY;
      }
      break;

    case WAIT_CONCURRENT_READY:
      Serial.print(F("Measurement state: "));
      Serial.println(
        static_cast<uint8_t>(mySDI12Recorder.measurementState(sensorAddress)));
      if (mySDI12Recorder.isMeasurementReady(sensorAddress)) {
        mySDI12Recorder.requestData(sensorAddress, dataValues, 9U);
        testStage = WAIT_CONCURRENT_DATA;
      }
      break;

    case WAIT_CONCURRENT_DATA:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Concurrent measurement data"));
      printDataValues();
      testStage = START_CONTINUOUS_DATA;
      break;

    case START_CONTINUOUS_DATA:
      mySDI12Recorder.startRequestContinuousData(sensorAddress, dataValues, 9U, 0U,
                                                 true);
      testStage = WAIT_CONTINUOUS_DATA;
      break;

    case WAIT_CONTINUOUS_DATA:
      if (mySDI12Recorder.isBusy()) { break; }
      printResult(F("Continuous CRC data"));
      printDataValues();
      testStage = START_RAW_COMMAND;
      break;

    case START_RAW_COMMAND:
      if (!testRawCommand) {
        testStage = TEST_COMPLETE;
        break;
      }
      mySDI12Recorder.startRawCommand(sensorAddress, String("X"));
      if (testCancellation) {
        mySDI12Recorder.cancel();
        printResult(F("Cancellation"));
        mySDI12Recorder.startRawCommand(sensorAddress, String("X"));
      }
      testStage = WAIT_RAW_COMMAND;
      break;

    case WAIT_RAW_COMMAND:
      if (mySDI12Recorder.isBusy()) {
        mySDI12Recorder.printPhase(Serial);
        Serial.println();
        break;
      }
      printResult(F("Raw command"));
      Serial.print(F("Response length: "));
      Serial.println(mySDI12Recorder.responseLength());
      Serial.print(F("Response pointer: "));
      Serial.println(mySDI12Recorder.response());
      Serial.print(F("Copied response length: "));
      Serial.println(
        mySDI12Recorder.copyResponse(copiedResponse, sizeof(copiedResponse)));
      mySDI12Recorder.clearResponse();
      testStage = TEST_COMPLETE;
      break;

    case TEST_COMPLETE: break;
  }
}
