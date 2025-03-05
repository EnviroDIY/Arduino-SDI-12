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

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

uint32_t serialBaud   = 115200; /*!< The baud rate for the output serial port */
int8_t   powerPin     = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
uint32_t wake_delay   = 0; /*!< Extra time needed for the sensor to wake (0-100ms) */
int8_t   firstPin     = 3; /*! change to lowest pin number to search on your board */
int8_t   lastPin      = 7; /*! change to highest pin number to search on your board */
int8_t   firstAddress = 0; /* The first address in the address space to check (0='0') */
int8_t   lastAddress = 61; /* The last address in the address space to check (61='z') */

/**
 * @brief converts allowable address characters ('0'-'9', 'a'-'z', 'A'-'Z') to a
 * decimal number between 0 and 61 (inclusive) to cover the 62 possible
 * addresses.
 */
byte charToDec(char i) {
  if ((i >= '0') && (i <= '9')) return i - '0';
  if ((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if ((i >= 'A') && (i <= 'Z'))
    return i - 'A' + 36;
  else
    return i;
}

/**
 * @brief maps a decimal number between 0 and 61 (inclusive) to allowable
 * address characters '0'-'9', 'a'-'z', 'A'-'Z',
 *
 * THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
 */
char decToChar(byte i) {
  if (i < 10) return i + '0';
  if ((i >= 10) && (i < 36)) return i + 'a' - 10;
  if ((i >= 36) && (i <= 62))
    return i + 'A' - 36;
  else
    return i;
}

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
  delay(30);

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
bool checkActive(SDI12& sdi, char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    sdi.clearBuffer();
    sdi.sendCommand(myCommand, wake_delay);
    Serial.print(">>>");
    Serial.println(myCommand);
    uint32_t start_millis = millis();
    while (!sdi.available() && millis() - start_millis < 250);
    if (sdi.available()) {  // If we hear anything, assume we have an active sensor
      Serial.print("<<<");
      while (sdi.available()) { Serial.write(sdi.read()); }
      Serial.println();
      return true;
    }
  }
  sdi.clearBuffer();
  return false;
}

void scanAddressSpace(SDI12& sdi) {
  // Quickly scan the address space
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    char addr = decToChar(i);
    Serial.print("i=");
    Serial.print(i);
    Serial.print(" addr=");
    Serial.println(addr);
    if (checkActive(sdi, addr)) { printInfo(sdi, addr); };
  }
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

  for (int8_t pin = firstPin; pin <= lastPin; pin++) {
    Serial.print("Checking pin ");
    Serial.print(pin);
    Serial.println("...");
    if (pin != powerPin) {
      pinMode(pin, INPUT);
      SDI12 mySDI12(pin);
      mySDI12.begin();
      scanAddressSpace(mySDI12);
      mySDI12.end();
    }
  }

  Serial.println("\n//\n// End Search for SDI-12 Devices \n// ---------------------");

  // Cut power
  digitalWrite(powerPin, LOW);
}

void loop() {}
