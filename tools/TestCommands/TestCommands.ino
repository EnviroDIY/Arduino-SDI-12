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

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */
#define FIRST_ADDRESS 0
#define LAST_ADDRESS 6  // 62
#define WAKE_DELAY 0    /*!< The extra time needed for the board to wake. */
#define COMMANDS_TO_TEST \
  2 /*!< The number of measurement commands to test, between 1 and 11. */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

/// variable that alternates output type back and forth between parsed and raw
boolean flip = 0;

String commands[] = {"","0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

// keeps track of active addresses
bool isActive[LAST_ADDRESS - FIRST_ADDRESS] = {
  0,
};

// keeps track of the wait time for each active addresses
uint8_t waitTime[LAST_ADDRESS - FIRST_ADDRESS] = {
  0,
};

// keeps track of the time each sensor was started
uint32_t millisStarted[LAST_ADDRESS - FIRST_ADDRESS] = {
  0,
};

// keeps track of the time each sensor will be ready
uint32_t millisReady[LAST_ADDRESS - FIRST_ADDRESS] = {
  0,
};

// keeps track of the number of results expected
uint8_t returnedResults[LAST_ADDRESS - FIRST_ADDRESS] = {
  0,
};


String prev_result[LAST_ADDRESS - FIRST_ADDRESS] = {
  "",
};
String this_result[LAST_ADDRESS - FIRST_ADDRESS] = {
  "",
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

bool getResults(char i, int resultsExpected, bool printCommands = true) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  // while (resultsReceived < resultsExpected && cmd_number <= 9) {
  while (resultsReceived < resultsExpected && cmd_number <= 1) {
    String command = "";
    // in this example we will only take the 'DO' measurement
    command = "";
    command += i;
    command += "D";
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
    } else {
      mySDI12.read();
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

String getStringResults(char i, int resultsExpected, bool printCommands = true) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  String  str_result      = "";
  while (resultsReceived < resultsExpected && cmd_number <= 9) {
    // while (resultsReceived < resultsExpected && cmd_number <= 1) {
    String command = "";
    // in this example we will only take the 'DO' measurement
    command = "";
    command += i;
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

    while (mySDI12.available()) {
      char c = mySDI12.peek();
      if (c == '-' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat(SKIP_NONE);
        str_result += String(result, 8);
        if (printCommands) { Serial.print(String(result, 8)); }
        if (result != -9999) { resultsReceived++; }
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
    if (!resultsReceived) { break; }  // don't do another loop if we got nothing
    cmd_number++;
  }
  mySDI12.clearBuffer();

  return str_result;
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
  delay(100);

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
  uint8_t wait = sdiResponse.substring(1, 4).toInt();
  if (printCommands) {
    Serial.print("wait: ");
    Serial.print(wait);
    Serial.print(", ");
  }

  // Set up the number of results to expect
  int numResults = sdiResponse.substring(4).toInt();
  if (printCommands) {
    Serial.print("Number Results: ");
    Serial.println(numResults);
  }

  uint8_t sensorNum   = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  waitTime[sensorNum] = wait;
  millisStarted[sensorNum] = millis();
  if (wait == 0) {
    millisReady[sensorNum] = millis();
  } else {
    millisReady[sensorNum] = millis() + (wait + 1 * 1000);
  }
  returnedResults[sensorNum] = numResults;

  return numResults;
}

String takeMeasurement(char i, String meas_type = "", bool printCommands = true) {
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
  uint8_t wait = sdiResponse.substring(1, 4).toInt();
  if (printCommands) {
    Serial.print("Wait: ");
    Serial.print(wait);
    Serial.print(", ");
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
  while ((millis() - timerStart) < ((uint16_t)1000 * (wait + 1))) {
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

  // return getResults(i, numResults,printCommands);
  String res = getStringResults(i, numResults, printCommands);
  Serial.print("Result: ");
  Serial.println(res);
  return res;
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

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering down sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, LOW);
    delay(2500L);
  }

  // Power the sensors;
  if (POWER_PIN > 0) {
    Serial.println("Powering up sensors...");
    pinMode(POWER_PIN, OUTPUT);
    digitalWrite(POWER_PIN, HIGH);
    delay(10000L);
    // delay(125);
  }

  // Quickly Scan the Address Space
  Serial.println("Scanning all addresses, please wait...");

  for (byte i = FIRST_ADDRESS; i < LAST_ADDRESS; i++) {
    char addr = decToChar(i);
    Serial.print("i: ");
    Serial.print(i);
    Serial.print(", addr: ");
    Serial.print(addr);
    Serial.print(", rev: ");
    Serial.print(charToDec(addr));
    if (checkActive(addr, 5, true)) {
      numSensors++;
      isActive[i] = 1;
      Serial.println(", +");
      printInfo(addr);
    } else {
      Serial.println(", -");
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
  // flip = !flip;  // flip the switch between concurrent and not
  // flip = 1;
  flip           = 0;
  uint32_t start = millis();
  Serial.print("Flip: ");
  Serial.println(flip);

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
    for (byte i = FIRST_ADDRESS; i < LAST_ADDRESS; i++) {
      char addr = decToChar(i);
      if (isActive[i]) {
        for (uint8_t a = 0; a < COMMANDS_TO_TEST; a++) {
          Serial.print("Command ");
          Serial.print(i);
          Serial.print("M");
          Serial.print(commands[a]);
          Serial.println('!');
          this_result[i] = takeMeasurement(addr, commands[a], true);
        }
        // getContinuousResults(addr, 3);
        Serial.println();
      }
    }
    Serial.print("Total Time for Individual Measurements: ");
    Serial.println(millis() - start);
  } else {
    for (uint8_t a = 0; a < COMMANDS_TO_TEST; a++) {
      uint8_t  min_wait  = 127;
      uint8_t  max_wait  = 0;
      uint32_t for_start = millis();
      // start all sensors measuring concurrently
      for (byte i = FIRST_ADDRESS; i < LAST_ADDRESS; i++) {
        char addr = decToChar(i);
        if (isActive[i]) {
          Serial.print("Command ");
          Serial.print(i);
          Serial.print("C");
          Serial.print(commands[a]);
          Serial.println('!');
          startConcurrentMeasurement(addr, commands[a], true);
        }
        if (waitTime[i] < min_wait) { min_wait = waitTime[i]; }
        if (waitTime[i] > max_wait) { max_wait = waitTime[i]; }
      }
      min_wait = max(1, (min_wait - 1) / 2);
      max_wait = max(1, max_wait + 1);
      // Serial.print("minimum expected wait: ");
      // Serial.println(min_wait);
      // Serial.print("maximum expected wait: ");
      // Serial.println(max_wait);


      uint8_t numReadingsRecorded = 0;
      delay(min_wait * 1000);
      while (millis() - for_start < max_wait * 1000 &&
             numReadingsRecorded < numSensors) {
        // get all readings
        for (byte i = FIRST_ADDRESS; i < LAST_ADDRESS; i++) {
          uint32_t timeWaited = millis() - millisStarted[i];
          if (this_result[i] != "") { prev_result[i] = this_result[i]; }

          char addr = decToChar(i);
          if (isActive[i]) {
            // if (millis() > millisReady[i]) {
            // if (millis() > millisStarted[i] + a) {
            if (returnedResults[i] > 0) {
              this_result[i] = getStringResults(addr, returnedResults[i], true);
              // if (this_result[i] != "") {
              // Serial.print("timeWaited: ");
              // Serial.print(timeWaited);
              // Serial.print(", This result: ");
              // Serial.println(this_result[i]);
              // Serial.print("                 , prev result: ");
              // Serial.println(prev_result[i]);
              // } else {
              //   Serial.print("timeWaited: ");
              //   Serial.println(timeWaited);
              // }
              // this_result = getResults(addr, returnedResults[i]);
              // Serial.print("Got results from ");
              // Serial.print(numReadingsRecorded);
              // Serial.print(" of ");
              // Serial.print(numSensors);
              // Serial.println(" sensors");
            }
            if (this_result[i] != prev_result[i] && this_result[i] != "") {
              numReadingsRecorded++;
              Serial.print("Time Waited: ");
              Serial.println(timeWaited);
              Serial.print("Result: ");
              Serial.println(this_result[i]);
            }
            // } else {
            //   Serial.print("Result from ");
            //   Serial.print(addr);
            //   Serial.print(" won't be ready for ");
            //   Serial.print(millisReady[i] - millis());
            //   Serial.println(" ms ");
            // }
          }
        }
      }
    }
    Serial.print("Total Time for Concurrent Measurements: ");
    Serial.println(millis() - start);
  }

  Serial.println("-------------------------------------------------------------------"
                 "------------");
  // delay(1000);  // wait ten seconds between measurement attempts.
}
