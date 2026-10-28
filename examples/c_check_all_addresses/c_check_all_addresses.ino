/**
 * @example{lineno} c_check_all_addresses.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example C: Check all Addresses for Active Sensors and Print Status
 *
 * This is a simple demonstration of the SDI-12 library for Arduino.
 *
 * It discovers the address of all sensors active and attached to the board.
 * THIS CAN BE *REALLY* SLOW TO RUN!!!
 *
 * Each sensor should have a unique address already - if not, multiple sensors may
 * respond simultaneously to the same request and the output will not be readable
 * by the Arduino.
 *
 * To address a sensor, please see Example B: b_address_change.ino
 */

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
uint32_t serialBaud   = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   powerPin     = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */
int8_t   firstPin     = 3; /*! change to lowest pin number to search on your board */
int8_t   lastPin      = 7; /*! change to highest pin number to search on your board */
int8_t   firstAddress = 0; /* The first address in the address space to check (0='0') */
int8_t   lastAddress = 61; /* The last address in the address space to check (61='z') */

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param recorder the SDI-12 recorder instance
 * @param address a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(SDI12Recorder& recorder, char address) {
  SDI12Recorder::Identification sensorInfo;
  if (recorder.identify(address, sensorInfo)) {
    recorder.printSensorInfo(sensorInfo, Serial);
  } else {
    recorder.printRecorderResult(Serial);
    Serial.println();
  }
}

void scanAddressSpace(SDI12Recorder& recorder) {
  // Quickly scan the address space
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    char addr = SDI12Address::fromIndex(i);
    Serial.print("i=");
    Serial.print(i);
    Serial.print(" addr=");
    Serial.println(addr);
    if (recorder.requestAcknowledgement(addr)) { printInfo(recorder, addr); }
  }
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);
  char tbuf[3] = {'\0'};

  Serial.println("//\n// Start Search for SDI-12 Devices \n// -----------------------");

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

  for (int8_t pin = firstPin; pin <= lastPin; pin++) {
    if (pin != powerPin) {
      Serial.print("Opening SDI-12 bus on pin ");
      Serial.print(pin);
      Serial.print(" (");
      Serial.print(itoa(pin, tbuf, 10));
      Serial.print(")");
      Serial.println("...");
      SDI12         mySDI12(pin);
      SDI12Recorder mySDI12Recorder(mySDI12);
      mySDI12.begin();
      delay(500);  // allow things to settle
      scanAddressSpace(mySDI12Recorder);
      mySDI12.end();
    }
  }

  Serial.println("\n//\n// End Search for SDI-12 Devices \n// ---------------------");

  // Cut power
  digitalWrite(powerPin, LOW);
}

void loop() {}
