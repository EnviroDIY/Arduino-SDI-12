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

#include <SDI12.h>

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

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(char i) {
  String command = "";
  command += (char)i;
  command += "I!";
  mySDI12.sendCommand(command, wake_delay);
  delay(30);

  String sdiResponse = mySDI12.readStringUntil('\n');
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
  Serial.print(sdiResponse.substring(20));  // sensor id
  Serial.print(", ");
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
bool checkActive(byte i) {  // this checks for activity at a particular address
  Serial.print("Checking address ");
  Serial.print((char)i);
  Serial.print("...");
  myCommand = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand, wake_delay);
    delay(30);
    if (mySDI12.available()) {  // If we hear anything, assume we have an active sensor
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
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);

  // Power the sensors;
  if (powerPin >= 0) {
    Serial.println("Powering up sensors, wait 30s...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(30000L);
  }
}

void loop() {
  boolean found = false;  // have we identified the sensor yet?

  for (byte i = '0'; i <= '9'; i++) {  // scan address space 0-9
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
      printInfo(i);
    }
  }

  for (byte i = 'a'; i <= 'z'; i++) {  // scan address space a-z
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
      printInfo(i);
    }
  }

  for (byte i = 'A'; i <= 'Z'; i++) {  // scan address space A-Z
    if (found) break;
    if (checkActive(i)) {
      found      = true;
      oldAddress = i;
      printInfo(i);
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
