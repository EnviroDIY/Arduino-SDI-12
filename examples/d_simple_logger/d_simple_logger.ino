/**
 * @example{lineno} d_simple_logger.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example D: Check all Addresses for Active Sensors and Log Data
 *
 * This is a simple demonstration of the SDI-12 library for Arduino.
 *
 * It discovers the address of all sensors active on a single bus and takes measurements
 * from them.
 *
 * Every SDI-12 device is different in the time it takes to take a
 * measurement, and the amount of data it returns. This sketch will not serve every
 * sensor type, but it will likely be helpful in getting you started.
 *
 * Each sensor should have a unique address already - if not, multiple sensors may
 * respond simultaneously to the same request and the output will not be readable by the
 * Arduino.
 *
 * To address a sensor, please see Example B: b_address_change.ino
 */

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

/* connection information */
uint32_t serialBaud   = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   dataPin      = SDI12_DATA_PIN;      /*!< The pin of the SDI-12 data bus */
int8_t   powerPin     = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */
uint32_t wake_delay   = 0; /*!< Extra time needed for the sensor to wake (0-100ms) */
int8_t   firstAddress = 0; /* The first address in the address space to check (0='0') */
int8_t   lastAddress = 61; /* The last address in the address space to check (61='z') */

/** Define the SDI-12 bus */
SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

// keeps track of active addresses
bool isActive[64];

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

bool takeMeasurement(char i, uint8_t measurementNumber = 0U) {
  float  values[9];
  size_t valueCount = mySDI12Recorder.measure(i, values, 9U, measurementNumber, false);
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    mySDI12Recorder.printRecorderResult(Serial);
    return false;
  }

  for (size_t valueNumber = 0U; valueNumber < valueCount; ++valueNumber) {
    Serial.print(String(values[valueNumber], 7));
    Serial.print(F(", "));
  }
  return true;
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
    } else {
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
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
  Serial.println(
    "Time Elapsed (ms), Sensor Address, Measurement 1, Measurement 2, ... etc.");
  Serial.println("-------------------------------------------------------------------"
                 "------------");
}

void loop() {
  for (uint8_t measurementNumber = 0U; measurementNumber < 1U; ++measurementNumber) {
    // measure one at a time
    for (int8_t i = firstAddress; i <= lastAddress; i++) {
      char addr = SDI12Address::fromIndex(i);
      if (isActive[i]) {
        Serial.print(millis());
        Serial.print(", ");
        Serial.print(addr);
        Serial.print(", ");
        takeMeasurement(addr, measurementNumber);
        Serial.println();
      }
    }
  }

  delay(10000L);  // wait ten seconds between measurement attempts.
}
