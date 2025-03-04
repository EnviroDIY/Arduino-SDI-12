/**
 * @example{lineno} k_concurrent_logger.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
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

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

/* connection information */
uint32_t serialBaud   = 115200;         /*!< The baud rate for the output serial port */
int8_t   dataPin      = SDI12_DATA_PIN; /*!< The pin of the SDI-12 data bus */
int8_t   powerPin     = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
uint32_t wake_delay   = 0; /*!< Extra time needed for the sensor to wake (0-100ms) */
int8_t   firstAddress = 0; /* The first address in the address space to check (0='0') */
int8_t   lastAddress = 61; /* The last address in the address space to check (61='z') */
bool     printIO     = false;

/** Define the SDI-12 bus */
SDI12 mySDI12(dataPin);

// keeps track of active addresses
bool isActive[64];

// keeps track of the wait time for each active addresses
uint8_t meas_time_ms[64];

// keeps track of the time each sensor was started
uint32_t millisStarted[64];

// keeps track of the time each sensor will be ready
uint32_t millisReady[64];

// keeps track of the number of results expected
uint8_t expectedResults[64];

// keeps track of the number of results returned
uint8_t returnedResults[64];

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
  mySDI12.sendCommand(command, wake_delay);
  delay(30);

  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  // allccccccccmmmmmmvvvxxx...xx<CR><LF>
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

bool getResults(char addr, int resultsExpected) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  uint8_t cmd_retries     = 0;

  // When requesting data, the sensor sends back up to ~80 characters at a
  // time to each data request.  If it needs to return more results than can
  // fit in the first data request (D0), we need to make additional requests
  // (D1-9).  Since this is a parent to all sensors, we're going to keep
  // requesting data until we either get as many results as we expect or no
  // more data is returned.
  while (resultsReceived < resultsExpected && cmd_number <= 9 && cmd_retries < 5) {
    bool    gotResults  = false;
    uint8_t cmd_results = 0;
    // Assemble the command based on how many commands we've already sent,
    // starting with D0 and ending with D9
    // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.clearBuffer();
    String command = "";
    command += addr;
    command += "D";
    command += cmd_number;
    command += "!";
    mySDI12.sendCommand(command, wake_delay);
    delay(30);
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }

    // Wait for the first few characters to arrive.  The response from a data
    // request should always have more than three characters
    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 1500) {}

    // read the returned address to remove it from the buffer
    char returnedAddress = mySDI12.read();
    if (printIO) {
      if (returnedAddress != addr) {
        Serial.println("Wrong address returned!");
        Serial.print("Expected ");
        Serial.print(String(addr));
        Serial.print(" Got ");
        Serial.println(String(returnedAddress));
      }
      Serial.print("<<<");
      Serial.write(returnedAddress);
      Serial.print(", ");
      Serial.println();
    }

    bool bad_read = false;
    // While there is any data left in the buffer
    while (mySDI12.available() && (millis() - start) < 3000) {
      char c = mySDI12.peek();
      // if there's a polarity sign, a number, or a decimal next in the
      // buffer, start reading it as a float.
      if (c == '-' || c == '+' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat();
        if (printIO) {
          Serial.print("<<<");
          Serial.println(String(result, 7));
        } else {
          Serial.print(String(result, 7));
          Serial.print(", ");
        }
        if (result != -9999) {
          gotResults = true;
          cmd_results++;
        }
        // if we get to a new line, we've made it to the end of the response
      } else if (c == '\r' || c == '\n') {
        if (printIO) { Serial.write(c); }
        mySDI12.read();
      } else {
        if (printIO) {
          Serial.print(F("<<< INVALID CHARACTER IN RESPONSE:"));
          Serial.write(c);
          Serial.println();
        }
        // Read the character to make sure it's removed from the buffer
        mySDI12.read();
        bad_read = true;
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (!gotResults) {
      if (printIO) {
        Serial.println(F("  No results received, will not continue requests!"));
        break;  // don't do another loop if we got nothing
      }
    }
    if (gotResults && !bad_read) {
      resultsReceived = resultsReceived + cmd_results;
      if (printIO) {
        Serial.print(F("  Total Results Received: "));
        Serial.print(resultsReceived);
        Serial.print(F(", Remaining: "));
        Serial.println(resultsExpected - resultsReceived);
      }
      cmd_number++;
    } else {
      // if we got a bad charater in the response, add one to the retry
      // attempts but do not bump up the command number or transfer any
      // results because we want to retry the same data command to try get
      // a valid response
      cmd_retries++;
    }
    mySDI12.clearBuffer();
  }

  return resultsReceived == resultsExpected;
}

int startConcurrentMeasurement(char i, String meas_type = "") {
  mySDI12.clearBuffer();
  String command = "";
  command += i;
  command += "C";
  command += meas_type;
  command += "!";  // SDI-12 concurrent measurement command format  [address]['C'][!]
  mySDI12.sendCommand(command, wake_delay);
  delay(30);

  if (printIO) {
    Serial.print(">>>");
    Serial.println(command);
  }

  // wait for acknowledgement with format [address][ttt (3 char, seconds)][number of
  // measurements available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  if (printIO) {
    Serial.print("<<<");
    Serial.println(sdiResponse);
  }

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

  if (printIO) { Serial.println(); }

  uint8_t sensorNum = charToDec(i);  // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61.
  meas_time_ms[sensorNum]  = wait;
  millisStarted[sensorNum] = millis();
  if (wait == 0) {
    millisReady[sensorNum] = millis();
  } else {
    millisReady[sensorNum] = millis() + wait * 1000;
  }
  expectedResults[sensorNum] = numResults;

  return numResults;
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
bool checkActive(char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand, wake_delay);
    if (printIO) {
      Serial.print(">>>");
      Serial.println(myCommand);
    }
    delay(30);
    if (mySDI12.available()) {  // If we hear anything, assume we have an active sensor
      if (printIO) {
        Serial.print("<<<");
        while (mySDI12.available()) { Serial.write(mySDI12.read()); }
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
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.println("Opening SDI-12 bus...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);

  // Power the sensors;
  if (powerPin >= 0) {
    Serial.println("Powering up sensors, wait...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(5000L);
  }

  // Quickly scan the address space
  Serial.println("Scanning all addresses, please wait...");
  Serial.println("Protocol Version, Sensor Address, Sensor Vendor, Sensor Model, "
                 "Sensor Version, Sensor ID");

  for (int8_t i = firstAddress; i <= lastAddress; i++) {
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
  Serial.println("Time Elapsed (s), Measurement 1, Measurement 2, ... etc.");
  Serial.println(
    "-------------------------------------------------------------------------------");
}

void loop() {
  // starting line
  if (printIO) { Serial.println("-------------"); }

  // re-zero before starting a new measurement round
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    millisReady[i]     = 0;
    expectedResults[i] = 0;
    returnedResults[i] = 0;
  }

  // start all sensors measuring concurrently
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    char addr = decToChar(i);
    if (isActive[i]) {
      startConcurrentMeasurement(addr);
      if (printIO) { Serial.println(); }
    }
  }

  // Wait for the first sensor to be ready
  uint32_t first_ready = millis() + 120000;
  for (int8_t i = firstAddress; i <= lastAddress; i++) {
    if (millisReady[i] != 0 && millisReady[i] < first_ready) {
      first_ready = millisReady[i];
    }
  }
  while ((millis() - first_ready) > 50) { delay(1); }

  // get all readings
  uint8_t numReadingsRecorded = 0;
  uint8_t round               = 1;

  do {
    if (printIO) {
      Serial.print("Round ");
      Serial.print(round);
      Serial.print(" sensors to read: ");
      Serial.print(numSensors);
      Serial.print(" finished sensors: ");
      Serial.print(numReadingsRecorded);
      Serial.println();
    }
    for (int8_t i = firstAddress; i <= lastAddress; i++) {
      if (isActive[i]) {
        char addr = decToChar(i);
        if (printIO) {
          Serial.print(addr);
          Serial.print(" - millis: ");
          Serial.print(millis());
          Serial.print(" ready at: ");
          Serial.print(millisReady[i]);
          Serial.print(" results expected: ");
          Serial.print(expectedResults[i]);
          Serial.print(" already returned: ");
          Serial.print(returnedResults[i]);
          Serial.println();
        }
        if (isActive[i] && millis() > millisReady[i] && expectedResults[i] > 0 &&
            (returnedResults[i] < expectedResults[i])) {
          if (!printIO) {
            Serial.print(millis() / 1000);
            Serial.print(", ");
            Serial.print(addr);
            Serial.print(", ");
          }
          getResults(addr, expectedResults[i]);
          numReadingsRecorded++;
          Serial.println();
        }
      }
    }
    round++;
  } while (numReadingsRecorded < numSensors);

  delay(10000L);  // wait ten seconds between measurement attempts.
}
