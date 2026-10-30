/**
 * @example{lineno} TestSensorTiming.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 * @date October 2026
 *
 * @brief Measures actual standard and concurrent measurement readiness.
 *
 * For standard measurements, this sketch measures from the validated measurement
 * acknowledgement until the sensor begins its service request. For concurrent
 * measurements, it starts a fresh measurement for each probe delay and finds the
 * earliest delay that repeatedly returns the complete, CRC-validated data set.
 * These values can guide the `_measurementTime_ms` used by ModularSensors.
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
#ifndef SDI12_SENSOR_WARM_UP_MS
#define SDI12_SENSOR_WARM_UP_MS 30000UL
#endif
#ifndef SDI12_STANDARD_TESTS
#define SDI12_STANDARD_TESTS 25U
#endif
#ifndef SDI12_CONCURRENT_PROBE_STEP_MS
#define SDI12_CONCURRENT_PROBE_STEP_MS 10UL
#endif
#ifndef SDI12_CONCURRENT_CONFIRMATIONS
#define SDI12_CONCURRENT_CONFIRMATIONS 3U
#endif
#ifndef SDI12_MAX_CONCURRENT_PROBE_MS
#define SDI12_MAX_CONCURRENT_PROBE_MS 180000UL
#endif

const uint32_t serialBaud    = SDI12_TERMINAL_BAUD;
const int8_t   dataPin       = SDI12_DATA_PIN;
const int8_t   powerPin      = SDI12_POWER_PIN;
const char     sensorAddress = SDI12_SENSOR_ADDRESS;

const uint8_t measurementNumber = 0U;
const bool    requestCRC        = true;
const size_t  valueCapacity     = 99U;

static_assert(SDI12_CONCURRENT_PROBE_STEP_MS > 0U,
              "SDI12_CONCURRENT_PROBE_STEP_MS must be greater than zero");
static_assert(SDI12_CONCURRENT_CONFIRMATIONS > 0U,
              "SDI12_CONCURRENT_CONFIRMATIONS must be greater than zero");

SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);
float         values[valueCapacity];

void waitForRecorder() {
  while (mySDI12Recorder.isBusy()) {
    mySDI12Recorder.update();
    yield();
  }
}

void printRecorderFailure(const __FlashStringHelper* operation) {
  Serial.print(operation);
  Serial.print(F(" failed: "));
  mySDI12Recorder.printRecorderResult(Serial);
  Serial.println();
}

void printValues(size_t valueCount) {
  Serial.print(F("Values:"));
  for (size_t i = 0U; i < valueCount; ++i) {
    Serial.print(i == 0U ? F(" ") : F(", "));
    Serial.print(values[i], 7);
  }
  Serial.println();
}

bool retrieveData(bool immediately, uint8_t expectedValues) {
  if (immediately) {
    mySDI12Recorder.requestDataImmediately(sensorAddress, values, valueCapacity);
  } else {
    mySDI12Recorder.requestData(sensorAddress, values, valueCapacity);
  }
  waitForRecorder();

  return mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS &&
    mySDI12Recorder.dataValueCount() == expectedValues;
}

bool testStandardMeasurement(uint8_t testNumber, uint32_t& readyMs) {
  mySDI12Recorder.startMeasurement(sensorAddress, measurementNumber, requestCRC);
  waitForRecorder();
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    printRecorderFailure(F("Standard measurement"));
    return false;
  }

  const uint16_t advertisedSeconds = mySDI12Recorder.measurementSeconds();
  const uint8_t  expectedValues    = mySDI12Recorder.expectedValueCount();
  const bool     serviceRequest = mySDI12Recorder.measurementServiceRequestReceived();
  readyMs = serviceRequest ? mySDI12Recorder.measurementServiceRequestMillis()
                           : static_cast<uint32_t>(advertisedSeconds) * 1000UL;

  const bool dataValid = retrieveData(false, expectedValues);

  Serial.print(F("Standard test "));
  Serial.print(testNumber);
  Serial.print(F(": advertised "));
  Serial.print(advertisedSeconds);
  Serial.print(F(" s; "));
  if (serviceRequest) {
    Serial.print(F("service request began after "));
    Serial.print(readyMs);
    Serial.print(F(" ms; "));
  } else {
    Serial.print(F("no service request; advertised time elapsed; "));
  }
  Serial.print(F("data "));
  Serial.println(dataValid ? F("valid") : F("invalid"));

  if (!dataValid) {
    printRecorderFailure(F("Standard data retrieval"));
    return false;
  }
  printValues(mySDI12Recorder.dataValueCount());
  return true;
}

bool probeConcurrentMeasurement(uint32_t probeDelayMs, uint32_t& advertisedMs,
                                uint32_t& dataCompleteMs) {
  advertisedMs   = static_cast<uint32_t>(-1);
  dataCompleteMs = 0U;
  mySDI12Recorder.startConcurrentMeasurement(sensorAddress, measurementNumber,
                                             requestCRC);
  waitForRecorder();
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    printRecorderFailure(F("Concurrent measurement"));
    return false;
  }

  advertisedMs = static_cast<uint32_t>(mySDI12Recorder.measurementSeconds()) * 1000UL;
  const uint8_t  expectedValues = mySDI12Recorder.expectedValueCount();
  const uint32_t measurementAcknowledgedAt =
    mySDI12Recorder.measurementAcknowledgementMillis();
  while (millis() - measurementAcknowledgedAt < probeDelayMs) { yield(); }

  const bool dataValid = retrieveData(true, expectedValues);
  dataCompleteMs       = millis() - measurementAcknowledgedAt;

  Serial.print(F("Concurrent probe at "));
  Serial.print(probeDelayMs);
  Serial.print(F(" ms: "));
  if (dataValid) {
    Serial.print(F("valid data complete after "));
    Serial.print(dataCompleteMs);
    Serial.println(F(" ms"));
    printValues(mySDI12Recorder.dataValueCount());
  } else {
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.print(F(" after "));
    Serial.print(dataCompleteMs);
    Serial.println(F(" ms"));
  }
  return dataValid;
}

bool findConcurrentReadyTime(uint32_t& confirmedProbeDelayMs) {
  uint32_t maxProbeDelay = SDI12_MAX_CONCURRENT_PROBE_MS;

  for (uint32_t probeDelay = 0U; probeDelay <= maxProbeDelay;
       probeDelay += SDI12_CONCURRENT_PROBE_STEP_MS) {
    uint32_t advertisedMs = 0U;
    uint32_t completedMs  = 0U;
    if (!probeConcurrentMeasurement(probeDelay, advertisedMs, completedMs)) {
      if (advertisedMs != static_cast<uint32_t>(-1) && advertisedMs < maxProbeDelay) {
        maxProbeDelay = advertisedMs;
      }
      if (probeDelay >= maxProbeDelay ||
          maxProbeDelay - probeDelay < SDI12_CONCURRENT_PROBE_STEP_MS) {
        break;
      }
      continue;
    }

    if (advertisedMs < maxProbeDelay) { maxProbeDelay = advertisedMs; }
    Serial.print(F("First successful concurrent probe was "));
    Serial.print(probeDelay);
    Serial.print(F(" ms after acknowledgement; confirming "));
    Serial.print(SDI12_CONCURRENT_CONFIRMATIONS);
    Serial.println(F(" times."));

    bool confirmed = true;
    for (uint8_t confirmation = 1U; confirmation < SDI12_CONCURRENT_CONFIRMATIONS;
         ++confirmation) {
      if (!probeConcurrentMeasurement(probeDelay, advertisedMs, completedMs)) {
        confirmed = false;
        break;
      }
    }
    if (confirmed) {
      confirmedProbeDelayMs = probeDelay;
      return true;
    }

    if (probeDelay >= maxProbeDelay ||
        maxProbeDelay - probeDelay < SDI12_CONCURRENT_PROBE_STEP_MS) {
      break;
    }
  }

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

  if (powerPin >= 0) {
    Serial.print(F("Powering sensor and waiting "));
    Serial.print(SDI12_SENSOR_WARM_UP_MS);
    Serial.println(F(" ms."));
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
  } else {
    Serial.print(F("Waiting "));
    Serial.print(SDI12_SENSOR_WARM_UP_MS);
    Serial.println(F(" ms for the externally powered sensor."));
  }
  delay(SDI12_SENSOR_WARM_UP_MS);

  if (!mySDI12Recorder.requestAcknowledgement(sensorAddress)) {
    printRecorderFailure(F("Sensor acknowledgement"));
    while (true) {}
  }

  SDI12Recorder::Identification identification;
  if (!mySDI12Recorder.identify(sensorAddress, identification)) {
    printRecorderFailure(F("Sensor identification"));
    while (true) {}
  }
  mySDI12Recorder.printSensorInfo(identification, Serial);
}

void loop() {
  Serial.println();
  Serial.println(F("================ Standard measurements ================"));
  uint32_t standardTotal = 0U;
  uint32_t standardMin   = static_cast<uint32_t>(-1);
  uint32_t standardMax   = 0U;
  uint8_t  standardValid = 0U;

  for (uint8_t test = 1U; test <= SDI12_STANDARD_TESTS; ++test) {
    uint32_t readyMs = 0U;
    if (!testStandardMeasurement(test, readyMs)) { continue; }
    standardTotal += readyMs;
    if (readyMs < standardMin) { standardMin = readyMs; }
    if (readyMs > standardMax) { standardMax = readyMs; }
    ++standardValid;
  }

  Serial.println();
  Serial.println(F("================ Standard summary ================"));
  Serial.print(F("Valid tests: "));
  Serial.print(standardValid);
  Serial.print(F(" of "));
  Serial.println(SDI12_STANDARD_TESTS);
  if (standardValid > 0U) {
    Serial.print(F("Minimum ready time: "));
    Serial.print(standardMin);
    Serial.println(F(" ms"));
    Serial.print(F("Mean ready time: "));
    Serial.print(standardTotal / standardValid);
    Serial.println(F(" ms"));
    Serial.print(F("Maximum ready time: "));
    Serial.print(standardMax);
    Serial.println(F(" ms"));
    Serial.print(F("Candidate standard _measurementTime_ms before safety margin: "));
    Serial.println(standardMax);
  }

  Serial.println();
  Serial.println(F("================ Concurrent measurements ================"));
  Serial.print(F("Probing in "));
  Serial.print(SDI12_CONCURRENT_PROBE_STEP_MS);
  Serial.println(F(" ms increments. Each early probe starts a fresh measurement."));
  Serial.println(F("Each data command is sent once; failed probes are not retried."));
  uint32_t concurrentDelay = 0U;
  if (findConcurrentReadyTime(concurrentDelay)) {
    Serial.print(F("Earliest confirmed concurrent data-request delay: "));
    Serial.print(concurrentDelay);
    Serial.println(F(" ms"));
    Serial.print(F("Candidate concurrent _measurementTime_ms before safety margin: "));
    Serial.println(concurrentDelay);
  } else {
    Serial.println(F("No repeatedly valid early concurrent data response was found."));
  }

  if (powerPin >= 0) { digitalWrite(powerPin, LOW); }
  Serial.println(F("FINISHED"));
  while (true) {}
}
