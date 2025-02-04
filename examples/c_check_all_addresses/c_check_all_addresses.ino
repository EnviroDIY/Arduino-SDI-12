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

#include <SDI12.h>

uint32_t serialBaud = 115200; /*!< The baud rate for the output serial port */
int8_t   powerPin   = 22; /*!< The sensor power pin (or -1 if not switching power) */
uint32_t wake_delay = 0;  /*!< Extra time needed for the sensor to wake (0-100ms) */
#define FirstPin 5        /*! change to lowest pin number on your board */
#define LastPin 24        /*! change to highest pin number on your board */

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port expects
 *
 * @param sdi the SDI-12 instance
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'
 */
void printInfo(SDI12& sdi, char i) {
  String command = "";
  command += (char)i;
  command += "I!";
  sdi.clearBuffer();
  sdi.sendCommand(command, wake_delay);
  delay(100);

  String sdiResponse = sdi.readStringUntil('\n');
  sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
  Serial.print(sdiResponse.substring(0, 1));  // address
  Serial.print(", ");
  Serial.print(sdiResponse.substring(1, 3).toFloat() / 10);  // SDI-12 version number
  Serial.print(", ");
  Serial.print(sdiResponse.substring(3, 11));  // vendor id
  Serial.print(", ");
  Serial.print(sdiResponse.substring(11, 17));  // sensor model
  Serial.print(", ");
  Serial.print(sdiResponse.substring(17, 20));  // sensor version
  Serial.print(", ");
  Serial.println(sdiResponse.substring(20));  // sensor id
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(SDI12& sdi, char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    sdi.clearBuffer();
    sdi.sendCommand(myCommand, wake_delay);
    delay(100);
    if (sdi.available()) {  // If we here anything, assume we have an active sensor
      sdi.clearBuffer();
      return true;
    }
  }
  sdi.clearBuffer();
  return false;
}

void scanAddressSpace(SDI12& sdi) {
  // scan address space 0-9
  for (char i = '0'; i <= '9'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); }
  // scan address space a-z
  for (char i = 'a'; i <= 'z'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); }
  // scan address space A-Z
  for (char i = 'A'; i <= 'Z'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); };
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println("//\n// Start Search for SDI-12 Devices \n// -----------------------");

  // Power the sensors;
  if (powerPin >= 0) {
    Serial.println("Powering up sensors, wait...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(5000L);
  }

  for (uint8_t pin = FirstPin; pin <= LastPin; pin++) {
    if (pin != powerPin) {
      pinMode(pin, INPUT);
      SDI12 mySDI12(pin);
      mySDI12.begin();
      Serial.print("Checking pin ");
      Serial.print(pin);
      Serial.println("...");
      scanAddressSpace(mySDI12);
      mySDI12.end();
    }
  }

  Serial.println("\n//\n// End Search for SDI-12 Devices \n// ---------------------");

  // Cut power
  digitalWrite(powerPin, LOW);
}

void loop() {}
