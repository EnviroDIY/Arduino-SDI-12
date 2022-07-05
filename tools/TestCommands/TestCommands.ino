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
void printInfo(char i, bool printCommands = true) {
  String command = "";
  command += (char)i;
  command += "I!";
  mySDI12.sendCommand(command, WAKE_DELAY);
  if (printCommands) {
    Serial.print(">>>");
    Serial.println(command);
  }
  delay(100);

  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
  if (printCommands) {
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
}

bool getResults(char addr, int resultsExpected, bool printCommands = true) {
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
    if (printCommands) {
      Serial.print(">>>");
      Serial.println(command);
    }

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 150) {}
    if (printCommands) {
      Serial.print("<<<");
      Serial.write(mySDI12.read());  // ignore the repeated SDI12 address
    } else {
      mySDI12.read();
    }

    bool gotResults = false;
    while (mySDI12.available()) {
      // First peek to see if the next character in the buffer is a number
      char c = mySDI12.peek();
      // if there's a number, a decimal, or a negative sign next in the
      // buffer, start reading it as a float.
      if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        // Read the float without skipping any in-valid characters.
        // We don't want to skip anything because we want to be able to
        // debug and see exactly which characters the sensor sent over
        // if they weren't numbers.
        // Reading the numbers as a float will remove them from the
        // buffer.
        float result = mySDI12.parseFloat(SKIP_NONE);
        // add result to print out string
        str_result += String(result, 8);
        // also print results if we're printing commands
        if (printCommands) { Serial.print(String(result, 8)); }
        // add how many results we have
        if (result != -9999) {
          gotResults = true;
          resultsReceived++;
        }
      } else if (c >= 0 && c != '\r' && c != '\n') {
        str_result += String(c);
        if (printCommands) {
          Serial.write(mySDI12.read());
        } else {
          mySDI12.read();
        }
      } else {
        mySDI12.read();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (printCommands) {
      Serial.print("\nTotal Results Received: ");
      Serial.print(resultsReceived);
      Serial.print(", Remaining: ");
      Serial.println(resultsExpected - resultsReceived);
    }
    if (!gotResults) {
      if (printCommands) {
        Serial.println(("  No results received, will not continue requests!"));
      }
      break;
    }  // don't do another loop if we got nothing
    cmd_number++;
  }
  mySDI12.clearBuffer();

  bool success = resultsReceived == resultsExpected;
  if (success) { this_result[charToDec(addr)] = str_result; }
  return success;
}

bool getContinuousResults(char i, int resultsExpected, bool printCommands = true) {
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
    if (printCommands) {
      Serial.print(">>>");
      Serial.println(command);
    }

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 1500) {}
    if (printCommands) {
      Serial.print("<<<");
      Serial.write(mySDI12.read());  // ignore the repeated SDI12 address
    }

    while (mySDI12.available()) {
      char c = mySDI12.peek();
      if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat(SKIP_NONE);
        Serial.print(String(result, 10));
        if (result != -9999) { resultsReceived++; }
      } else if (c >= 0 && c != '\r' && c != '\n') {
        Serial.write(mySDI12.read());
      } else {
        mySDI12.read();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (printCommands) {
      Serial.print("Total Results Received: ");
      Serial.print(resultsReceived);
      Serial.print(", Remaining: ");
      Serial.println(resultsExpected - resultsReceived);
    }
    if (!resultsReceived) { break; }  // don't do another loop if we got nothing
    cmd_number++;
  }
  mySDI12.clearBuffer();

  return resultsReceived == resultsExpected;
}

int startConcurrentMeasurement(char i, String meas_type = "",
                               bool printCommands = true) {
  String command = "";
  command += i;
  command += "C";
  command += meas_type;
  command += "!";  // SDI-12 concurrent measurement command format  [address]['C'][!]
  mySDI12.sendCommand(command, WAKE_DELAY);
  if (printCommands) {
    Serial.print(">>>");
    Serial.println(command);
  }
  delay(5);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  if (printCommands) {
    Serial.print("<<<");
    Serial.println(sdiResponse);
  }
  mySDI12.clearBuffer();

  // find out how long we have to wait (in seconds).
  uint8_t meas_time_s = sdiResponse.substring(1, 4).toInt();
  if (printCommands) {
    Serial.print("expected measurement time: ");
    Serial.print(meas_time_s);
    Serial.print(" s, ");
  }

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();
  if (printCommands) {
    Serial.print("Number Results: ");
    Serial.println(numResults);
  }

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

bool takeMeasurement(char i, String meas_type = "", bool printCommands = true) {
  String command = "";
  command += i;
  command += "M";
  command += meas_type;
  command += "!";  // SDI-12 measurement command format  [address]['M'][!]
  mySDI12.sendCommand(command, WAKE_DELAY);
  if (printCommands) {
    Serial.print(">>>");
    Serial.println(command);
  }
  delay(100);

  // wait for acknowlegement with format [address][ttt (3 char, seconds)][number of
  // measurments available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  if (printCommands) {
    Serial.print("<<<");
    Serial.println(sdiResponse);
  }

  // find out how long we have to wait (in seconds).
  uint32_t meas_time_s = sdiResponse.substring(1, 4).toInt();
  if (printCommands) {
    Serial.print("Sensor Reported Measurement Time: ");
    Serial.print(meas_time_s);
    Serial.print(" s, ");
  }

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();
  if (printCommands) {
    Serial.print("Number Results: ");
    Serial.println(numResults);
  }
  // if (numResults==0){return false;}
  if (numResults == 0) { return ""; }

  unsigned long timerStart = millis();
  while ((millis() - timerStart) < (meas_time_s + 1) * 1000) {
    if (mySDI12.available())  // sensor can interrupt us to let us know it is done early
    {
      unsigned long measTime = millis() - timerStart;
      if (printCommands) {
        Serial.print("<<<");
        Serial.println(mySDI12.readStringUntil('\n'));
        // mySDI12.clearBuffer();
      }
      Serial.print("Completed after ");
      Serial.print(measTime);
      Serial.println(" ms");
      break;
    }
  }
  // Wait for anything else and clear it out
  delay(30);
  mySDI12.clearBuffer();

  return getResults(i, numResults, printCommands);
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i, int8_t numPings = 3, bool printCommands = false) {
  String command = "";
  command += (char)i;  // sends basic 'acknowledge' command [address][!]
  command += "!";

  for (int j = 0; j < numPings; j++) {  // goes through three rapid contact attempts
    if (printCommands) {
      Serial.print(">>>");
      Serial.println(command);
    }
    mySDI12.sendCommand(command, WAKE_DELAY);
    delay(100);
    if (mySDI12.available()) {  // If we here anything, assume we have an active sensor
      if (printCommands) {
        Serial.print("<<<");
        while (mySDI12.available()) {
          Serial.write(mySDI12.read());
          delay(10);
        }
      } else {
        mySDI12.clearBuffer();
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
    Serial.print("i: ");
    Serial.print(i);
    Serial.print(", addr: ");
    Serial.print(addr);
    Serial.print(", reversed: ");
    Serial.println(charToDec(addr));
    if (checkActive(addr, 5, true)) {
      numSensors++;
      isActive[i] = 1;
      // Serial.println(", +");
      printInfo(addr, true);
    } else {
      // Serial.println(", -");
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
  flip = !flip;  // flip the switch between concurrent and not
  // flip = 1;
  // flip           = 0;
  uint32_t start = millis();
  // Serial.print("Flip: ");
  // Serial.println(flip);

  // // Power the sensors;
  // if (POWER_PIN > 0) {
  //   Serial.println("Powering down sensors...");
  //   pinMode(POWER_PIN, OUTPUT);
  //   digitalWrite(POWER_PIN, LOW);
  //   delay(5000L);
  // }

  // // Power the sensors;
  // if (POWER_PIN > 0) {
  //   Serial.println("Powering up sensors...");
  //   pinMode(POWER_PIN, OUTPUT);
  //   digitalWrite(POWER_PIN, HIGH);
  //   delay(125);
  // }

  if (flip) {
    // measure one at a time
    for (int8_t i = 0; i < 62; i++) {
      char addr = decToChar(i);
      if (isActive[i]) {
        for (uint8_t a = 0; a < COMMANDS_TO_TEST; a++) {
          Serial.print("Command ");
          Serial.print(i);
          Serial.print("M");
          Serial.print(commands[a]);
          Serial.println('!');
          takeMeasurement(addr, commands[a], true);
        }
        // getContinuousResults(addr, 3);
        Serial.println();
      } else {
        Serial.print("Address ");
        Serial.print(addr);
        Serial.println(" is not active");
      }
    }
    Serial.print("Total Time for Individual Measurements: ");
    Serial.println(millis() - start);
  } else {
    for (uint8_t a = 0; a < COMMANDS_TO_TEST; a++) {
      uint32_t min_wait  = 60000L;
      uint32_t max_wait  = 0;
      uint32_t for_start = millis();
      // start all sensors measuring concurrently
      for (int8_t i = 0; i < 62; i++) {
        char addr = decToChar(i);
        if (isActive[i]) {
          Serial.print("Command ");
          Serial.print(i);
          Serial.print("C");
          Serial.print(commands[a]);
          Serial.println('!');
          startConcurrentMeasurement(addr, commands[a], true);
          if (meas_time_ms[i] < min_wait) { min_wait = meas_time_ms[i]; }
          if (meas_time_ms[i] > max_wait) { max_wait = meas_time_ms[i]; }
        } else {
          Serial.print("Address ");
          Serial.print(addr);
          Serial.println(" is not active");
        }
      }
      min_wait = 800;
      // min_wait = max(10, min_wait / 2);
      max_wait = max(1000L, max_wait + 1000L);
      Serial.print("minimum expected wait for all sensors: ");
      Serial.println(min_wait);
      Serial.print("maximum expected wait for all sensors: ");
      Serial.println(max_wait);


      uint8_t numReadingsRecorded = 0;
      delay(min_wait);
      do {
        // get all readings
        for (int8_t i = 0; i < 62; i++) {
          uint32_t timeWaited = millis() - millisStarted[i];
          if (this_result[i] != "") { prev_result[i] = this_result[i]; }

          char addr = decToChar(i);
          if (isActive[i]) {
            // if (millis() > millisReady[i]) {
            // if (millis() > millisStarted[i] + a) {
            if (returnedResults[i] > 0) {
              Serial.print("timeWaited: ");
              Serial.println(timeWaited);
              bool resultsReady = getResults(addr, returnedResults[i], true);
              if (resultsReady) {
                numReadingsRecorded++;
                Serial.print("Got results from ");
                Serial.print(numReadingsRecorded);
                Serial.print(" of ");
                Serial.print(numSensors);
                Serial.println(" sensors");
              }
            }
          }
        }
      } while (millis() - for_start < max_wait && numReadingsRecorded < numSensors);
    }
    Serial.print("Total Time for Concurrent Measurements: ");
    Serial.println(millis() - start);
  }

  Serial.println("-------------------------------------------------------------------"
                 "------------");
  // delay(1000);  // wait ten seconds between measurement attempts.
}
