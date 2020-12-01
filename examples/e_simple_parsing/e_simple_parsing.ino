/**
 * @file e_simple_parsing.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example E: Parsing Integers and Floats from the Buffer
 *
 *  This example demonstrates the ability to parse integers and floats from the buffer.
 * It is based closely on example D, however, every other time it prints out data, it
 * multiplies the data by a factor of 2.
 *
 * Time Elapsed (s), Sensor Address and ID, Measurement 1, Measurement 2, ... etc.
 * -------------------------------------------------------------------------------
 * 6,c13SENSOR ATM    311,0.62 x 2 = 1.24,19.80 x 2 = 39.60
 * 17,c13SENSOR ATM   311,0.62,19.7
 * 29,c13SENSOR ATM   311,0.62 x 2 = 1.2419.70 x 2 = 39.40
 * 41,c13SENSOR ATM   311,0.62,19.8
 *
 * This is a trivial and nonsensical example, but it does demonstrate the ability to
 * manipulate incoming data.
 */

#include <SDI12.h>

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

/// variable that alternates output type back and forth between parsed and raw
boolean flip = 1;

// The code below alternates printing in non-parsed, and parsed mode.
//
// The parseInt() and parseFloat() functions will timeout if they do not
// find a candidate INT or FLOAT.  The value returned when a such a timeout is
// encountered is set in SDI12.cpp by default to -9999.  You can change the
// default setting directly with the setTimeoutValue function:
//       mySDI12.setTimeoutValue(int)
// The value should not be a possible data value.
//
// You should always check for timeouts before interpreting data, as
// shown in the example below.

// keeps track of active addresses
bool isActive[64] = {
  0,
};

uint8_t numSensors = 0;


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
 * @brief Parses numbers out of the buffer
 */
void parseMeasurements() {
  mySDI12.read();  // discard address
  while (mySDI12.available()) {
    float that = mySDI12.parseFloat();
    if (that != mySDI12.TIMEOUT) {  // check for timeout
      float doubleThat = that * 2;
      Serial.print(", ");
      Serial.print(that);
      Serial.print(" x 2 = ");
      Serial.print(doubleThat);
    } else {
      Serial.print(", TIMEOUT:");
      Serial.print(that);
    }
  }
  Serial.println();
}

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
  mySDI12.sendCommand(command);
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

bool takeMeasurement(char i, String meas_type = "") {
  mySDI12.clearBuffer();
  String command = "";
  command += i;
  command += "M";
  command += meas_type;
  command += "!";  // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command);
  delay(100);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();

  String addr = sdiResponse.substring(0, 1);
  Serial.print(addr);
  Serial.print(", ");

  // find out how long we have to wait (in seconds).
  uint8_t wait = sdiResponse.substring(1, 4).toInt();
  Serial.print(wait);
  Serial.print(", ");

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();
  Serial.print(numResults);
  Serial.print(", ");

  unsigned long timerStart = millis();
  while ((millis() - timerStart) < (1000 * (wait + 1))) {
    if (mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      Serial.print(millis() - timerStart);
      Serial.print(", ");
      mySDI12.clearBuffer();
      break;
    }
  }
  // Wait for anything else and clear it out
  delay(30);
  mySDI12.clearBuffer();

  if (numResults > 0) { return getResults(i, numResults); }

  return true;
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    delay(100);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}


void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial)
    ;

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(200);
  }

  // Quickly Scan the Address Space
  Serial.println("Scanning all addresses, please wait...");
  Serial.println("Protocol Version, Sensor Address, Sensor Vendor, Sensor Model, "
                 "Sensor Version, Sensor ID");

  for (byte i = 0; i < 62; i++) {
    char addr = decToChar(i);
    if (checkActive(addr)) {
      numSensors++;
      isActive[i] = 1;
      printInfo(addr);
      Serial.println();
    }
  }
  Serial.print("Total number of sensors found:  ");
  Serial.println(numSensors);

  if (numSensors == 0) {
    Serial.println(
      "No sensors found, please check connections and restart the Arduino.");
    while (true) { delay(10); }  // do nothing forever
  }

  Serial.println();
  Serial.println("Time Elapsed (s), Est Measurement Time (s), Number Measurements, "
                 "Real Measurement Time (ms), Measurement 1, Measurement 2, ... etc.");
  Serial.println(
    "-------------------------------------------------------------------------------");
}

void loop() {
  flip = !flip;  // flip the switch between parsing and not parsing
  // measure one at a time
  for (byte i = 0; i < 62; i++) {
    char addr = decToChar(i);
    if (isActive[i]) {
      Serial.print(millis() / 1000);
      Serial.print(", ");
      takeMeasurement(addr);
      Serial.println();
    }
  }

  delay(10000);  // wait ten seconds between measurement attempts.
}
