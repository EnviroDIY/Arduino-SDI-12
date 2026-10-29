/**
 * @example{lineno} k_concurrent_logger.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Example K:  Concurrent Measurements
 *
 * This is very similar to example B - finding all attached sensors and logging data
 * from them. Unlike example B, however, which waits for each sensor to complete a
 * measurement, this asks all sensors to take measurements concurrently and then waits
 * until each is finished to query for results. This can be much faster than waiting for
 * each sensor when you have multiple sensor attached.
 */

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
#ifndef SDI12_PRINT_IO
#define SDI12_PRINT_IO false
#endif

/* connection information */
uint32_t serialBaud   = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   dataPin      = SDI12_DATA_PIN;      /*!< The pin of the SDI-12 data bus */
int8_t   powerPin     = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */
bool     printIO      = SDI12_PRINT_IO;
uint32_t wake_delay   = 0; /*!< Extra time needed for the sensor to wake (0-100ms) */
int8_t   firstAddress = 0; /* The first address in the address space to check (0='0') */
int8_t   lastAddress = 61; /* The last address in the address space to check (61='z') */

/** Define the SDI-12 bus */
SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

// keeps track of active addresses
bool isActive[64];

// keeps track of the wait time for each active addresses
uint8_t meas_time_ms[64];

// keeps track of the time each sensor was started
uint32_t millisStarted[64];

// keeps track of the time each sensor will be ready
uint32_t millisReady[64];

// keeps track of the number of results expected
uint8_t expectedResults[64];

// keeps track of the number of results returned
uint8_t returnedResults[64];

uint8_t numSensors = 0;

bool printInfo(char address) {
  SDI12Recorder::Identification sensorInfo;
  if (mySDI12Recorder.identify(address, sensorInfo)) {
    mySDI12Recorder.printSensorInfo(sensorInfo, Serial);
    return true;
  } else {
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }
  return false;
}

bool getResults(char address, int resultsExpected) {
  if (resultsExpected < 0 || resultsExpected > 99) { return false; }

  float values[99];
  mySDI12Recorder.requestData(address, values, 99U);
  while (mySDI12Recorder.isBusy()) {
    mySDI12Recorder.update();
    yield();
  }
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    if (printIO) {
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
    }
    return false;
  }

  const size_t valueCount = mySDI12Recorder.dataValueCount();
  for (size_t valueNumber = 0U; valueNumber < valueCount; ++valueNumber) {
    Serial.print(String(values[valueNumber], 7));
    Serial.print(F(", "));
  }
  return valueCount == static_cast<size_t>(resultsExpected);
}

int startConcurrentMeasurement(char i, uint8_t measurementNumber = 0U) {
  mySDI12Recorder.startConcurrentMeasurement(i, measurementNumber, false);
  while (mySDI12Recorder.isBusy()) {
    mySDI12Recorder.update();
    yield();
  }
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    if (printIO) {
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
    }
    return 0;
  }

  const uint8_t sensorNum  = static_cast<uint8_t>(SDI12Address::toIndex(i));
  meas_time_ms[sensorNum]  = mySDI12Recorder.measurementSeconds();
  millisStarted[sensorNum] = millis();
  millisReady[sensorNum]   = millis() +
    static_cast<uint32_t>(mySDI12Recorder.measurementSeconds()) * 1000UL;
  expectedResults[sensorNum] = mySDI12Recorder.expectedValueCount();
  return expectedResults[sensorNum];
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);
  char tbuf[3] = {'\0'};

  Serial.print("Opening SDI-12 bus on pin ");
  Serial.print(dataPin);
  Serial.print(" (");
  Serial.print(itoa(dataPin, tbuf, 10));
  Serial.print(")");
  Serial.println("...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);

  // Power the sensors;
  if (powerPin >= 0) {
    Serial.print("Powering up sensors with pin ");
    Serial.print(String(powerPin));
    Serial.println(", wait 10s...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(10000L);
  } else {
    Serial.println("Wait 5s...");
    delay(5000L);
  }

  // Quickly scan the address space
  Serial.println("\n\nScanning all addresses, please wait...");

  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    char addr = SDI12Address::fromIndex(i);
    if (mySDI12Recorder.requestAcknowledgement(addr)) {
      numSensors++;
      isActive[i] = 1;
      printInfo(addr);
    }
  }
  Serial.print("Total number of sensors found:  ");
  Serial.println(numSensors);

  if (numSensors == 0) {
    Serial.println(
      "No sensors found, please check connections and restart the Arduino.");
    while (true);  // do nothing forever
  }

  Serial.println();
  Serial.println("Time Elapsed (s), Measurement 1, Measurement 2, ... etc.");
  Serial.println(
    "-------------------------------------------------------------------------------");
}

void loop() {
  // starting line
  if (printIO) { Serial.println("-------------"); }

  // re-zero before starting a new measurement round
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    millisReady[i]     = 0;
    expectedResults[i] = 0;
    returnedResults[i] = 0;
  }

  // Start as many sensors as the configured recorder context table can retain.
  uint8_t sensorsStarted = 0U;
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    char addr = SDI12Address::fromIndex(i);
    if (isActive[i]) {
      if (startConcurrentMeasurement(addr) > 0) { ++sensorsStarted; }
      if (printIO) { Serial.println(); }
    }
  }

  // Wait for the first sensor to be ready
  uint32_t first_ready = millis() + 120000;
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    if (millisReady[i] != 0 && millisReady[i] < first_ready) {
      first_ready = millisReady[i];
    }
  }
  while ((millis() - first_ready) > 50) { delay(1); }

  // get all readings
  uint8_t numReadingsRecorded = 0;
  uint8_t round               = 1;

  do {
    if (printIO) {
      Serial.print("Round ");
      Serial.print(round);
      Serial.print(" sensors to read: ");
      Serial.print(sensorsStarted);
      Serial.print(" finished sensors: ");
      Serial.print(numReadingsRecorded);
      Serial.println();
    }
    for (int8_t i = firstAddress; i <= lastAddress; i++) {
      if (isActive[i]) {
        char addr = SDI12Address::fromIndex(i);
        if (printIO) {
          Serial.print(addr);
          Serial.print(" - millis: ");
          Serial.print(millis());
          Serial.print(" ready at: ");
          Serial.print(millisReady[i]);
          Serial.print(" results expected: ");
          Serial.print(expectedResults[i]);
          Serial.print(" already returned: ");
          Serial.print(returnedResults[i]);
          Serial.println();
        }
        if (isActive[i] && mySDI12Recorder.isMeasurementReady(addr) &&
            expectedResults[i] > 0 && (returnedResults[i] < expectedResults[i])) {
          if (!printIO) {
            Serial.print(millis() / 1000);
            Serial.print(", ");
            Serial.print(addr);
            Serial.print(", ");
          }
          getResults(addr, expectedResults[i]);
          numReadingsRecorded++;
          Serial.println();
        }
      }
    }
    round++;
  } while (numReadingsRecorded < sensorsStarted);

  delay(10000L);  // wait ten seconds between measurement attempts.
}
