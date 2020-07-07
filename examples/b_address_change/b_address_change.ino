/**
 * @file b_address_change.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example B: Changing the Address of your SDI-12 Sensor
 *
 * This is a simple demonstration of the SDI-12 library for arduino.
 * It discovers the address of the attached sensor and allows you to change it.
 */

#include <SDI12.h>

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

String myCommand  = "";   // empty to start
char   oldAddress = '!';  // invalid address as placeholder


// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(byte i) {  // this checks for activity at a particular address
  Serial.print("Checking address ");
  Serial.print((char)i);
  Serial.print("...");
  myCommand = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(30);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      Serial.println("Occupied");
      mySDI12.clearBuffer();
      return true;
    } else {
      Serial.println("Vacant");  // otherwise it is vacant.
      mySDI12.clearBuffer();
    }
  }
  return false;
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial)
    ;

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
  }
}

void loop() {
  boolean found = false;  // have we identified the sensor yet?

  for (byte i = '0'; i <= '9'; i++) {  // scan address space 0-9
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
    }
  }

  for (byte i = 'a'; i <= 'z'; i++) {  // scan address space a-z
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
    }
  }

  for (byte i = 'A'; i <= 'Z'; i++) {  // scan address space A-Z
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
    }
  }

  if (!found) {
    Serial.println(
      "No sensor detected. Check physical connections.");  // couldn't find a sensor.
                                                           // check connections..
  } else {
    Serial.print("Sensor active at address ");  // found a sensor!
    Serial.print(oldAddress);
    Serial.println(".");

    Serial.println("Enter new address.");  // prompt for a new address
    while (!Serial.available())
      ;
    char newAdd = Serial.read();

    // wait for valid response
    while (((newAdd < '0') || (newAdd > '9')) && ((newAdd < 'a') || (newAdd > 'z')) &&
           ((newAdd < 'A') || (newAdd > 'Z'))) {
      if (!(newAdd == '\n') || (newAdd == '\r') || (newAdd == ' ')) {
        Serial.println(
          "Not a valid address. Please enter '0'-'9', 'a'-'A', or 'z'-'Z'.");
      }
      while (!Serial.available())
        ;
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

    Serial.println("Success. Rescanning for verification.");
  }
}
