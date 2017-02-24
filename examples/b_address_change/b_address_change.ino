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

#include <SDI12_PCINT3.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

uint32_t serialBaud = 57600;           /*!< The baud rate for the output serial port */
int8_t   dataPin    = SDI12_DATA_PIN;  /*!< The pin of the SDI-12 data bus */
int8_t   powerPin   = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
uint32_t wake_delay = 0; /*!< Extra time needed for the sensor to wake (0-100ms) */

/** Define the SDI-12 bus */
SDI12 mySDI12(dataPin);

String myCommand  = "";   // empty to start
char   oldAddress = '!';  // invalid address as placeholder

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
bool checkActive(char address, int8_t numPings = 3, bool printIO = false) {
  String command = "";
  command += (char)address;  // sends basic 'acknowledge' command [address][!]
  command += "!";

  for (int j = 0; j < numPings; j++) {  // goes through rapid contact attempts
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }
    mySDI12.clearBuffer();
    mySDI12.sendCommand(command, wake_delay);
    // wait for a response; the sensor must return within 100ms
    uint32_t waitStart = millis();
    while ((millis() - waitStart) < 150 && !mySDI12.available()) { yield(); }

    // the sensor should just return its address followed by '\r\n'
    if (mySDI12.available()) {
      String sdiResponse_r = mySDI12.readStringUntil('\n');
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
      mySDI12.clearBuffer();

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

  mySDI12.clearBuffer();
  return false;
}

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param address a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 * @param printIO true to print the raw output and input from the command
 */
bool printInfo(char address, bool printIO = true) {
  String command = "";
  command += (char)address;
  command += "I!";
  mySDI12.sendCommand(command, wake_delay);
  if (printIO) {
    Serial.print(">>>");
    Serial.println(command);
  }

  String sdiResponse = mySDI12.readStringUntil('\n');
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
  boolean found = false;  // have we identified the sensor yet?

  for (byte i = '0'; i <= '9'; i++) {  // scan address space 0-9
    if (found) break;
    Serial.print("Checking address ");
    Serial.print((char)i);
    Serial.print("...");
    if (checkActive(i)) {
      Serial.println("Occupied");
      found      = true;
      oldAddress = i;
      printInfo(i);
    } else {
      Serial.println("Vacant");
    }
  }

  for (byte i = 'a'; i <= 'z'; i++) {  // scan address space a-z
    if (found) break;
    Serial.print("Checking address ");
    Serial.print((char)i);
    Serial.print("...");
    if (checkActive(i)) {
      Serial.println("Occupied");
      found      = true;
      oldAddress = i;
      printInfo(i);
    } else {
      Serial.println("Vacant");
    }
  }

  for (byte i = 'A'; i <= 'Z'; i++) {  // scan address space A-Z
    if (found) break;
    Serial.print("Checking address ");
    Serial.print((char)i);
    Serial.print("...");
    if (checkActive(i)) {
      Serial.println("Occupied");
      found      = true;
      oldAddress = i;
      printInfo(i);
    } else {
      Serial.println("Vacant");
    }
  }

  if (!found) {
    Serial.println(
      "No sensor detected. Check physical connections.");  // couldn't find a sensor.
                                                           // check connections..
    while (1)                                              // die
      ;
  } else {
    Serial.print("Sensor active at address ");  // found a sensor!
    Serial.print(oldAddress);
    Serial.println(".");

    Serial.println("Enter new address.");  // prompt for a new address
    while (!Serial.available());
    char newAdd = Serial.read();

    // wait for valid response
    while (((newAdd < '0') || (newAdd > '9')) && ((newAdd < 'a') || (newAdd > 'z')) &&
           ((newAdd < 'A') || (newAdd > 'Z'))) {
      if (!(newAdd == '\n') || (newAdd == '\r') || (newAdd == ' ')) {
        Serial.println(
          "Not a valid address. Please enter '0'-'9', 'a'-'A', or 'z'-'Z'.");
      }
      while (!Serial.available());
      newAdd = Serial.read();
    }

    /* the syntax of the change address command is:
    [currentAddress]A[newAddress]! */

    Serial.println("Readdressing sensor.");
    myCommand = "";
    myCommand += (char)oldAddress;
    myCommand += "A";
    myCommand += (char)newAdd;
    myCommand += "!";
    mySDI12.sendCommand(myCommand);

    /* wait for the response then throw it away by
    clearing the buffer with clearBuffer()  */
    delay(300);
    mySDI12.clearBuffer();

    Serial.println("Success. Re-scanning for verification.");
  }
}
