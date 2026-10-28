/**
 * @example{lineno} b_address_change.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example B: Changing the Address of your SDI-12 Sensor
 *
 * This is a simple demonstration of the SDI-12 library for arduino.
 * It discovers the address of the attached sensor and allows you to change it.
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
/* connection information */
uint32_t serialBaud = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   dataPin    = SDI12_DATA_PIN;      /*!< The pin of the SDI-12 data bus */
int8_t   powerPin   = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */

/** Define the SDI-12 bus and recorder */
SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

char oldAddress = '!';  // invalid address as placeholder

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param address a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(char address) {
  SDI12Recorder::Identification sensorInfo;
  if (mySDI12Recorder.identify(address, sensorInfo)) {
    mySDI12Recorder.printSensorInfo(sensorInfo, Serial);
  } else {
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }
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
  }
}

void loop() {
  // The wildcard query works only when exactly one sensor is attached to the bus.
  oldAddress = mySDI12Recorder.queryAddress();
  if (oldAddress == '\0') {
    Serial.print("No sensor detected: ");
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
    Serial.println("Check physical connections and restart the Arduino.");
    while (true);
  }

  Serial.print("Sensor active at address ");
  Serial.print(oldAddress);
  Serial.println(".");
  printInfo(oldAddress);

  Serial.println("Enter new address.");  // prompt for a new address
  while (!Serial.available());
  char newAdd = Serial.read();

  // wait for valid response
  while (!SDI12Address::isValid(newAdd)) {
    if (newAdd != '\n' && newAdd != '\r' && newAdd != ' ') {
      Serial.println("Not a valid address. Please enter '0'-'9', 'a'-'z', or 'A'-'Z'.");
    }
    while (!Serial.available());
    newAdd = Serial.read();
  }

  Serial.println("Readdressing sensor.");
  if (mySDI12Recorder.changeAddress(oldAddress, newAdd)) {
    Serial.println("Success. Querying the sensor again for verification.");
  } else {
    Serial.print("Address change failed: ");
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
    Serial.println("Querying the bus again.");
  }
}
