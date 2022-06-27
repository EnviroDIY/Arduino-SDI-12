/**
 * @file d_simple_logger.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 *
 * @brief Example D: Check all Addresses for Active Sensors and Log Data
 *
 * This is a simple demonstration of the SDI-12 library for Arduino.
 *
 * It discovers the address of all sensors active on a single bus and takes measurements
 * from them.
 *
 * Every SDI-12 device is different in the time it takes to take a
 * measurement, and the amount of data it returns. This sketch will not serve every
 * sensor type, but it will likely be helpful in getting you started.
 *
 * Each sensor should have a unique address already - if not, multiple sensors may
 * respond simultaenously to the same request and the output will not be readable by the
 * Arduino.
 *
 * To address a sensor, please see Example B: b_address_change.ino
 */

#include <SDI12.h>

uint32_t SERIAL_BAUD   = 115200; /*!< The baud rate for the output serial port */
uint8_t  DATA_PIN      = 7;      /*!< The pin of the SDI-12 data bus */
uint8_t  POWER_PIN     = 22; /*!< The sensor power pin (or -1 if not switching power) */
int8_t   FIRST_ADDRESS = 1;
int8_t   LAST_ADDRESS  = 6;  // 62
int8_t   WAKE_DELAY    = 0;  /*!< The extra time needed for the board to wake. */
int8_t   COMMANDS_TO_TEST =
  1; /*!< The number of measurement commands to test, between 1 and 11. */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

/// variable that alternates output type back and forth between parsed and raw
boolean flip = 0;

String commands[] = {"", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

// keeps track of active addresses
bool isActive[62];

// keeps track of the wait time for each active addresses
uint32_t meas_time_ms[62];

// keeps track of the time each sensor was started
uint32_t millisStarted[62];

// keeps track of the time each sensor will be ready
uint32_t millisReady[62];

// keeps track of the number of results expected
uint8_t returnedResults[62];

String  prev_result[62];
String  this_result[62];
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
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(char i) {
  String command = "";
  command += (char)i;
  command += "I!";
  mySDI12.sendCommand(command, WAKE_DELAY);
  Serial.print("\n>>>");
  Serial.println(command);
  delay(15);

  String sdiResponse = mySDI12.readStringUntil('\n');
  // sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
  Serial.print("\n<<<");
  Serial.println(sdiResponse);
}

bool getResults(char addr, int resultsExpected) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  String  str_result      = "";
  while (resultsReceived < resultsExpected && cmd_number <= 9) {
    // while (resultsReceived < resultsExpected && cmd_number <= 1) {
    String command = "";
    // in this example we will only take the 'DO' measurement
    command = "";
    command += addr;
    command += "D";
    command += cmd_number;
    command += "!";  // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.sendCommand(command, WAKE_DELAY);
    Serial.print("\n>>>");
    Serial.println(command);

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 150) {}
    Serial.print("\n<<<");
    Serial.write(mySDI12.read());  // ignore the repeated SDI12 address

    uint8_t char_rcvd = 0;
    while (mySDI12.available()) {
      char_rcvd++;
      char c = mySDI12.peek();
      if (c == '+') {
        Serial.write(mySDI12.read());
        resultsReceived++;
      } else if (c >= 0 && c != '\r' && c != '\n') {
        Serial.write(mySDI12.read());
      } else {
        mySDI12.read();
        Serial.println();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (char_rcvd <= 2) {
      break;
    }  // don't do another loop if didn't get more than the address
    cmd_number++;
  }
  mySDI12.clearBuffer();

  bool success = resultsReceived == resultsExpected;
  if (success) { this_result[charToDec(addr)] = str_result; }
  return success;
}

bool getContinuousResults(char i, int resultsExpected) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  while (resultsReceived < resultsExpected && cmd_number <= 9) {
    String command = "";
    // in this example we will only take the 'DO' measurement
    command = "";
    command += i;
    command += "R";
    command += cmd_number;
    command += "!";  // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.sendCommand(command, WAKE_DELAY);
    Serial.print("\n>>>");
    Serial.println(command);

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 1500) {}
    Serial.print("\n<<<");
    Serial.write(mySDI12.read());  // ignore the repeated SDI12 address

    while (mySDI12.available()) {
      char c = mySDI12.peek();
      if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat(SKIP_NONE);
        Serial.print(String(result, 10));
        if (result != -9999) { resultsReceived++; }
      } else if (c >= 0 && c != '\r' && c != '\n') {
        Serial.write(mySDI12.read());
      } else {
        // mySDI12.read();
        Serial.println();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (!resultsReceived) { break; }  // don't do another loop if we got nothing
    cmd_number++;
  }
  mySDI12.clearBuffer();

  return resultsReceived == resultsExpected;
}

int startConcurrentMeasurement(char i, String meas_type = "") {
  String command = "";
  command += i;
  command += "C";
  command += meas_type;
  command += "!";  // SDI-12 concurrent measurement command format  [address]['C'][!]
  mySDI12.sendCommand(command, WAKE_DELAY);
  Serial.print("\n>>>");
  Serial.println(command);
  delay(5);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  Serial.print("\n<<<");
  Serial.println(sdiResponse);
  mySDI12.clearBuffer();

  // find out how long we have to wait (in seconds).
  uint8_t meas_time_s = sdiResponse.substring(1, 4).toInt();

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();

  uint8_t sensorNum = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  meas_time_ms[sensorNum]  = ((uint32_t)(meas_time_s)) * 1000;
  millisStarted[sensorNum] = millis();
  if (meas_time_ms == 0) {
    millisReady[sensorNum] = millis();
  } else {
    // give an extra second
    millisReady[sensorNum] = millis() + meas_time_ms[sensorNum] + 1000;
  }
  returnedResults[sensorNum] = numResults;

  return numResults;
}

bool takeMeasurement(char i, String meas_type = "") {
  String command = "";
  command += i;
  command += "M";
  command += meas_type;
  command += "!";  // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command, WAKE_DELAY);
  Serial.print("\n>>>");
  Serial.println(command);
  delay(15);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  Serial.print("\n<<<");
  Serial.println(sdiResponse);

  // find out how long we have to wait (in seconds).
  uint32_t meas_time_s = sdiResponse.substring(1, 4).toInt();

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();
  // if (numResults==0){return false;}
  if (numResults == 0) { return ""; }

  unsigned long timerStart = millis();
  while ((millis() - timerStart) < (meas_time_s + 1) * 1000) {
    if (mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      unsigned long measTime = millis() - timerStart;
      Serial.print("\n<<<");
      Serial.println(mySDI12.readStringUntil('\n'));
      break;
    }
  }
  // Wait for anything else and clear it out
  delay(30);
  mySDI12.clearBuffer();

  return getResults(i, numResults);
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i, int8_t numPings = 3) {
  String command = "";
  command += (char)i;  // sends basic 'acknowledge' command [address][!]
  command += "!";

  for (int j = 0; j < numPings; j++) {  // goes through three rapid contact attempts
    Serial.print("\n>>>");
    Serial.println(command);
    mySDI12.sendCommand(command, WAKE_DELAY);
    delay(15);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      Serial.print("\n<<<");
      uint8_t i = 0;
      while (mySDI12.available() && i < 60) {
        Serial.write(mySDI12.read());
        delay(5);
        i++;
      }
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

  for (int8_t i = 0; i < 62; i++) {
    isActive[i]        = false;
    meas_time_ms[i]    = 0;
    millisStarted[i]   = 0;
    millisReady[i]     = 0;
    returnedResults[i] = 0;
    prev_result[i]     = "";
    this_result[i]     = "";
  }

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering down sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
    // delay(2500L);
    delay(250L);
  }

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering up sensors, wait...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    // delay(10000L);
    delay(125);
  }

  // Quickly Scan the Address Space
  Serial.println("Scanning all addresses, please wait...");

  for (int8_t i = FIRST_ADDRESS; i <= LAST_ADDRESS; i++) {
    char addr = decToChar(i);
    if (checkActive(addr, 5)) {
      numSensors++;
      isActive[i] = 1;
      printInfo(addr);
    } else {
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
  Serial.println("-------------------------------------------------------------------"
                 "------------");

  delay(1000);
}

void loop() {
  uint32_t start = millis();
  // measure one at a time
  for (int8_t i = 0; i < 62; i++) {
    char addr = decToChar(i);
    if (isActive[i]) {
      printInfo(addr);
      for (uint8_t a = 0; a < COMMANDS_TO_TEST; a++) {
        takeMeasurement(addr, commands[a]);
      }
      Serial.println();
    }
  }

  Serial.println("-------------------------------------------------------------------"
                 "------------");
  delay(1000);  // wait ten seconds
}
