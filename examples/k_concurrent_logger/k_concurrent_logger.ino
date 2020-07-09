/**
 * @file k_concurrent_logger.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Sara Geleskie Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Example K:  Concurrent Measurements
 *
 * This is very similar to example B - finding all attached sensors and logging data
 * from them. Unlike example B, however, which waits for each sensor to complete a
 * measurement, this asks all sensors to take measurements concurrently and then waits
 * until each is finished to query for results. This can be much faster than waiting for
 * each sensor when you have multiple sensor attached.
 */


#include <SDI12.h>

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

/**
 * @brief keeps track of active addresses
 * each bit represents an address:
 * 1 is active (taken), 0 is inactive (available)
 * setTaken('A') will set the proper bit for sensor 'A'
 */
byte addressRegister[8] = {0B00000000, 0B00000000, 0B00000000, 0B00000000,
                           0B00000000, 0B00000000, 0B00000000, 0B00000000};

// keeps track of the wait time for each active addresses
uint8_t waitTime[64] = {
  0,
};

// keeps track of the time each sensor was started
uint32_t millisStarted[64] = {
  0,
};

// keeps track of the time each sensor will be ready
uint32_t millisReady[64] = {
  0,
};

uint8_t numSensors = 0;


// converts allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
// to a decimal number between 0 and 61 (inclusive) to cover the 62 possible addresses
byte charToDec(char i) {
  if ((i >= '0') && (i <= '9')) return i - '0';
  if ((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if ((i >= 'A') && (i <= 'Z'))
    return i - 'A' + 37;
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
  if (i <= 9) return i + '0';
  if ((i >= 10) && (i <= 36)) return i + 'a' - 10;
  if ((i >= 37) && (i <= 62))
    return i + 'A' - 37;
  else
    return i;
}

void printBufferToScreen() {
  String buffer = "";
  mySDI12.read();  // consume address
  while (mySDI12.available()) {
    char c = mySDI12.read();
    if (c == '+') {
      buffer += ',';
    } else if ((c != '\n') && (c != '\r')) {
      buffer += c;
    }
    delay(50);
  }
  Serial.print(buffer);
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
  // Serial.print(">>>");
  // Serial.println(command);
  delay(30);

  printBufferToScreen();
}

void startConcurrentMeasurement(char i) {
  String command = "";
  command += i;
  command += "C!";  // SDI-12 concurrent measurement command format  [address]['C'][!]
  mySDI12.sendCommand(command);
  // Serial.print(">>>");
  // Serial.println(command);
  delay(30);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = "";
  delay(30);
  while (mySDI12.available())  // build response string
  {
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(5);
    }
  }
  // Serial.print("<<<");
  // Serial.println(sdiResponse);
  mySDI12.clearBuffer();

  // find out how long we have to wait (in seconds).
  uint8_t wait = 0;
  wait         = sdiResponse.substring(1, 4).toInt();

  uint8_t sensorNum   = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  waitTime[sensorNum] = wait;
  millisStarted[sensorNum] = millis();
  millisReady[sensorNum]   = millis() + wait * 1000;
}

void getResults(char i) {
  String command = "";
  // in this example we will only take the 'DO' measurement
  command = "";
  command += i;
  command += "D0!";  // SDI-12 command to get data [address][D][dataOption][!]
  mySDI12.sendCommand(command);
  // Serial.print(">>>");
  // Serial.println(command);

  while (!(mySDI12.available() > 1)) {}  // wait for acknowlegement
  delay(300);                            // let the data transfer
  printBufferToScreen();
  mySDI12.clearBuffer();
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
    delay(30);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      printBufferToScreen();
      mySDI12.clearBuffer();
      return true;
    }
  }
  mySDI12.clearBuffer();
  return false;
}

// this quickly checks if the address has already been taken by an active sensor
boolean isTaken(byte i) {
  i      = charToDec(i);                 // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j = i / 8;                        // byte #
  byte k = i % 8;                        // bit #
  return addressRegister[j] & (1 << k);  // return bit status
}

// this sets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setTaken(byte i) {
  boolean initStatus = isTaken(i);
  i                  = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j             = i / 8;         // byte #
  byte k             = i % 8;         // bit #
  addressRegister[j] |= (1 << k);
  return !initStatus;  // return false if already taken
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL.
// this unsets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken.
boolean setVacant(byte i) {
  boolean initStatus = isTaken(i);
  i                  = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  byte j             = i / 8;         // byte #
  byte k             = i % 8;         // bit #
  addressRegister[j] &= ~(1 << k);
  return initStatus;  // return false if already vacant
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

  Serial.println("Scanning all addresses, please wait...");
  /*
      Quickly Scan the Address Space
   */

  for (byte i = '0'; i <= '9'; i++)
    if (checkActive(i)) {
      numSensors++;
      setTaken(i);
    }  // scan address space 0-9

  for (byte i = 'a'; i <= 'z'; i++)
    if (checkActive(i)) {
      numSensors++;
      setTaken(i);
    }  // scan address space a-z

  for (byte i = 'A'; i <= 'Z'; i++)
    if (checkActive(i)) {
      numSensors++;
      setTaken(i);
    }  // scan address space A-Z

  /*
      See if there are any active sensors.
   */
  boolean found = false;

  for (byte i = 0; i < 62; i++) {
    if (isTaken(i)) {
      found = true;
      Serial.print("First address found:  ");
      Serial.println(decToChar(i));
      Serial.print("Total number of sensors found:  ");
      Serial.println(numSensors);
      break;
    }
  }

  if (!found) {
    Serial.println(
      "No sensors found, please check connections and restart the Arduino.");
    while (true) { delay(10); }  // do nothing forever
  }

  Serial.println();
  Serial.println(
    "Time Elapsed (s), Sensor Address and ID, Measurement 1, Measurement 2, ... etc.");
  Serial.println(
    "-------------------------------------------------------------------------------");
}

void loop() {
  // start all sensors
  for (char i = '0'; i <= '9'; i++)
    if (isTaken(i)) { startConcurrentMeasurement(i); }
  for (char i = 'a'; i <= 'z'; i++)
    if (isTaken(i)) { startConcurrentMeasurement(i); }
  for (char i = 'A'; i <= 'Z'; i++)
    if (isTaken(i)) { startConcurrentMeasurement(i); }

  // get all readings
  uint8_t numReadingsRecorded = 0;
  while (numReadingsRecorded < numSensors) {
    for (char i = '0'; i <= '9'; i++) {
      if (isTaken(i)) {
        if (millis() > millisReady[charToDec(i)]) {
          Serial.print(millis() / 1000);
          Serial.print(",\t");
          getResults(i);
          Serial.print(",\t(");
          printInfo(i);
          Serial.print(")");
          Serial.println();
          numReadingsRecorded++;
        }
      }
    }
    for (char i = 'a'; i <= 'z'; i++) {
      if (isTaken(i)) {
        if (millis() > millisReady[charToDec(i)]) {
          Serial.print(millis() / 1000);
          Serial.print(",\t");
          getResults(i);
          Serial.print(",\t(");
          printInfo(i);
          Serial.print(")");
          Serial.println();
          numReadingsRecorded++;
        }
      }
    }
    for (char i = 'A'; i <= 'Z'; i++) {
      if (isTaken(i)) {
        if (millis() > millisReady[charToDec(i)]) {
          Serial.print(millis() / 1000);
          Serial.print(",\t");
          getResults(i);
          Serial.print(",\t(");
          printInfo(i);
          Serial.print(")");
          Serial.println();
          numReadingsRecorded++;
        }
      }
    }
  }

  delay(10000);  // wait ten seconds between measurement attempts.
}
