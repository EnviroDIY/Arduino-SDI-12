/**
 * @example{lineno} m_non_blocking_concurrent_logger.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Codex, at instruction of Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Example M: Non-blocking concurrent measurements.
 *
 * This example discovers sensors, starts concurrent measurements, and retrieves each
 * result when ready. Every recorder transaction is advanced from loop() with update().
 */

#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && not defined(SDI12_INTERNAL_PCINT)
#include <EnableInterrupt.h>
#endif

#include <SDI12.h>
#include <SDI12Protocol.h>
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
const int8_t   firstAddress  = 0;
const int8_t   lastAddress   = 7;
const uint8_t  addressCount  = lastAddress - firstAddress + 1;
const bool     useCRC        = true;
const uint32_t loggingPeriod = 10000UL;

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

bool  isActive[lastAddress + 1]           = {};
bool  measurementPending[lastAddress + 1] = {};
float values[99];

enum LoggerPhase : uint8_t { SCAN_BUS, START_MEASUREMENTS, READ_MEASUREMENTS, PAUSE };

enum PendingOperation : uint8_t {
  NO_OPERATION,
  ACKNOWLEDGEMENT,
  IDENTIFICATION,
  CONCURRENT_MEASUREMENT,
  DATA_RETRIEVAL
};

LoggerPhase      loggerPhase      = SCAN_BUS;
PendingOperation pendingOperation = NO_OPERATION;
int8_t           nextAddress      = firstAddress;
int8_t           startAddress     = firstAddress;
int8_t           pendingIndex     = -1;
uint8_t          sensorsFound     = 0U;
uint8_t          sensorsStarted   = 0U;
uint8_t          sensorsRead      = 0U;
uint8_t          addressesChecked = 0U;
uint32_t         roundEndMillis   = 0U;

int8_t nextAddressAfter(int8_t address) {
  return address < lastAddress ? address + 1 : firstAddress;
}

int8_t nextActiveAddressAfter(int8_t address) {
  for (uint8_t i = 0U; i < addressCount; ++i) {
    address = nextAddressAfter(address);
    if (isActive[address]) { return address; }
  }
  return firstAddress;
}

void printOperationFailure(char address) {
  Serial.print(F("Address "));
  Serial.print(address);
  Serial.print(F(": "));
  mySDI12Recorder.printRecorderResult(Serial);
  Serial.println();
}

void printValues(char address) {
  Serial.print(millis() / 1000UL);
  Serial.print(F(", "));
  Serial.print(address);
  for (size_t i = 0U; i < mySDI12Recorder.dataValueCount(); ++i) {
    Serial.print(F(", "));
    Serial.print(values[i], 7);
  }
  Serial.println();
}

void handleCompletedOperation() {
  const char address = SDI12Address::fromIndex(pendingIndex);
  const bool success = mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS;

  switch (pendingOperation) {
    case ACKNOWLEDGEMENT:
      if (success) {
        isActive[pendingIndex] = true;
        ++sensorsFound;
        mySDI12Recorder.startIdentify(address);
        pendingOperation = IDENTIFICATION;
        return;
      }
      ++nextAddress;
      break;

    case IDENTIFICATION:
      if (success) {
        mySDI12Recorder.printSensorInfo(mySDI12Recorder.identification(), Serial);
      } else {
        printOperationFailure(address);
      }
      ++nextAddress;
      break;

    case CONCURRENT_MEASUREMENT:
      if (success) {
        measurementPending[pendingIndex] = true;
        ++sensorsStarted;
      } else {
        printOperationFailure(address);
        if (mySDI12Recorder.result() == SDI12Recorder::RESULT_CONTEXT_CAPACITY) {
          loggerPhase = READ_MEASUREMENTS;
        }
      }
      nextAddress = nextAddressAfter(nextAddress);
      ++addressesChecked;
      break;

    case DATA_RETRIEVAL:
      if (success) {
        printValues(address);
      } else {
        printOperationFailure(address);
      }
      measurementPending[pendingIndex] = false;
      ++sensorsRead;
      break;

    case NO_OPERATION: break;
  }

  pendingOperation = NO_OPERATION;
  pendingIndex     = -1;
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

  Serial.println(F("Scanning the SDI-12 bus without blocking loop()."));
}

void loop() {
  mySDI12Recorder.update();

  if (mySDI12Recorder.isBusy()) { return; }
  if (pendingOperation != NO_OPERATION) { handleCompletedOperation(); }
  if (pendingOperation != NO_OPERATION) { return; }

  switch (loggerPhase) {
    case SCAN_BUS:
      if (nextAddress <= lastAddress) {
        pendingIndex = nextAddress;
        mySDI12Recorder.startRequestAcknowledgement(
          SDI12Address::fromIndex(pendingIndex));
        pendingOperation = ACKNOWLEDGEMENT;
      } else {
        Serial.print(F("Sensors found: "));
        Serial.println(sensorsFound);
        startAddress     = nextActiveAddressAfter(lastAddress);
        nextAddress      = startAddress;
        addressesChecked = 0U;
        loggerPhase      = START_MEASUREMENTS;
      }
      break;

    case START_MEASUREMENTS:
      while (addressesChecked < addressCount && !isActive[nextAddress]) {
        nextAddress = nextAddressAfter(nextAddress);
        ++addressesChecked;
      }
      if (addressesChecked < addressCount) {
        pendingIndex = nextAddress;
        mySDI12Recorder.startConcurrentMeasurement(
          SDI12Address::fromIndex(pendingIndex), 0U, useCRC);
        pendingOperation = CONCURRENT_MEASUREMENT;
      } else {
        loggerPhase = READ_MEASUREMENTS;
      }
      break;

    case READ_MEASUREMENTS:
      if (sensorsRead >= sensorsStarted) {
        roundEndMillis = millis();
        loggerPhase    = PAUSE;
        break;
      }
      for (int8_t i = firstAddress; i <= lastAddress; ++i) {
        const char address = SDI12Address::fromIndex(i);
        if (measurementPending[i] && mySDI12Recorder.isMeasurementReady(address)) {
          pendingIndex = i;
          mySDI12Recorder.requestData(address, values, 99U);
          pendingOperation = DATA_RETRIEVAL;
          break;
        }
      }
      break;

    case PAUSE:
      if (millis() - roundEndMillis >= loggingPeriod) {
        for (int8_t i = firstAddress; i <= lastAddress; ++i) {
          measurementPending[i] = false;
        }
        sensorsStarted   = 0U;
        sensorsRead      = 0U;
        startAddress     = nextActiveAddressAfter(startAddress);
        nextAddress      = startAddress;
        addressesChecked = 0U;
        loggerPhase      = START_MEASUREMENTS;
      }
      break;
  }
}
