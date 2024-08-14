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
#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && not defined(SDI12_INTERNAL_PCINT)
#include <EnableInterrupt.h>
#endif
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

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
bool checkActive(SDI12& sdi, char address, int8_t numPings = 3, bool printIO = false) {
  sdi.clearBuffer();
  String command = "";
  command += (char)address;  // sends basic 'acknowledge' command [address][!]
  command += "!";

  for (int j = 0; j < numPings; j++) {  // goes through rapid contact attempts
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }
    sdi.sendCommand(command, wake_delay);
    // wait for a response; the sensor must return within 100ms
    uint32_t waitStart = millis();
    while ((millis() - waitStart) < 150 && !sdi.available()) { yield(); }

    // the sensor should just return its address followed by '\r\n'
    if (sdi.available()) {
      String sdiResponse_r = sdi.readStringUntil('\n');
      String sdiResponse   = sdiResponse_r;
      int    resLen        = sdiResponse.length();
      if (printIO) {
        sdiResponse_r.replace("\r", "←");
        sdiResponse_r.replace("\n", "↓");
        Serial.print("<<< '");
        Serial.print(sdiResponse_r);
        Serial.print("' (");
        Serial.print(resLen);
        Serial.println(")");
      }
      sdi.clearBuffer();

      // check the address, return false if it's incorrect
      String returnedAddress = sdiResponse.substring(0, 1);
      if (printIO) {
        Serial.print("returnedAddress '");
        Serial.print(returnedAddress);
        Serial.print("' (");
        Serial.print(returnedAddress.length());
        Serial.println(")");
      }
      if (returnedAddress == String(address)) {
        if (printIO) {
          Serial.print("Got response from '");
          Serial.print(String(returnedAddress));
          Serial.println("'");
        }
        return true;
      } else {
        if (printIO) {
          Serial.println("Wrong address returned!");
          Serial.print("Expected '");
          Serial.print(String(address));
          Serial.print("' Got '");
          Serial.print(returnedAddress);
          Serial.println("'");
          // Serial.println(sdiResponse);
        }
      }
    }
  }

  sdi.clearBuffer();
  return false;
}

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param sdi the SDI-12 instance
 * @param address a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 * @param printIO true to print the raw output and input from the command
 */
bool printInfo(SDI12& sdi, char address, bool printIO = true) {
  String command = "";
  command += (char)address;
  command += "I!";
  sdi.sendCommand(command, wake_delay);
  if (printIO) {
    Serial.print(">>>");
    Serial.println(command);
  }

  String sdiResponse = sdi.readStringUntil('\n');
  sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
  if (printIO) {
    Serial.print("<<<");
    Serial.println(sdiResponse);
  }

  Serial.print("Address: ");
  Serial.print(sdiResponse.substring(0, 1));  // address
  Serial.print(", SDI-12 Version: ");
  Serial.print(sdiResponse.substring(1, 3).toFloat() / 10);  // SDI-12 version number
  Serial.print(", Vendor ID: ");
  Serial.print(sdiResponse.substring(3, 11));  // vendor id
  Serial.print(", Sensor Model: ");
  Serial.print(sdiResponse.substring(11, 17));  // sensor model
  Serial.print(", Sensor Version: ");
  Serial.print(sdiResponse.substring(17, 20));  // sensor version
  Serial.print(", Sensor ID: ");
  Serial.print(sdiResponse.substring(20));  // sensor id
  Serial.println();

  if (sdiResponse.length() < 3) { return false; };
  return true;
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
      SDI12 mySDI12(pin);
      mySDI12.begin();
      delay(500);  // allow things to settle
      scanAddressSpace(mySDI12);
      mySDI12.end();
    }
  }

  Serial.println("\n//\n// End Search for SDI-12 Devices \n// ---------------------");

  // Cut power
  digitalWrite(powerPin, LOW);
}

void loop() {}
