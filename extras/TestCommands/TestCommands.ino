/**
 * @example{lineno} TestCommands.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 */

#include <SDI12.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

// #define TEST_PRINT_ARRAY

/* connection information */
#if F_CPU > 48000000L
uint32_t serialBaud = 921600; /*!< The baud rate for the output serial port */
#else
uint32_t serialBaud = 115200; /*!< The baud rate for the output serial port */
#endif
int8_t       dataPin    = SDI12_DATA_PIN;  /*!< The pin of the SDI-12 data bus */
int8_t       powerPin   = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
uint32_t     wake_delay = 10; /*!< Extra time needed for the sensor to wake (0-100ms) */
const int8_t firstAddress =
  0; /* The first address in the address space to check (0='0') */
const int8_t lastAddress =
  61; /* The last address in the address space to check (61='z') */
const int8_t commandsToTest =
  1; /*!< The number of measurement commands to test, between 1 and 11. */

/** Define the SDI-12 bus */
SDI12 mySDI12(dataPin);

/** Define some testing specs */
const int8_t n_addresses = (lastAddress - firstAddress) + 1;

/** Error codes, if returned */
int8_t error_result_number = 7;
float  no_error_value      = 0;

/// variable that alternates output type back and forth between parsed and raw
boolean flip = 0;

String commands[] = {"", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

// keeps track of active addresses
bool isActive[n_addresses];

// keeps track of the wait time for each active addresses
uint32_t meas_time_ms[n_addresses];

// keeps track of the time each sensor was started
uint32_t millisStarted[n_addresses];

// keeps track of the time each sensor will be ready
uint32_t millisReady[n_addresses];

// keeps track of the number of results expected
uint8_t expectedResults[n_addresses];

// keeps track of the number of results returned
uint8_t returnedResults[n_addresses];

String  prev_result[n_addresses];
String  this_result[n_addresses];
uint8_t numSensors = 0;

struct startMeasurementResult {  // Structure declaration
  String  returnedAddress;
  uint8_t meas_time_s;
  int     numberResults;
};

struct getResultsResult {  // Structure declaration
  uint8_t resultsReceived;
  uint8_t maxDataCommand;
  bool    addressMatch;
  bool    crcMatch;
  bool    errorCode;
  bool    success;
};

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
  delay(30);

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

getResultsResult getResults(char address, int resultsExpected, bool verify_crc = false,
                            bool printIO = true) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  uint8_t cmd_retries     = 0;

  // From SDI-12 Protocol v1.4, Section 4.4 SDI-12 Commands and Responses:
  // The maximum number of characters that can be returned in the <values> part of the
  // response to a D command is either 35 or 75. If the D command is issued to
  // retrieve data in response to a concurrent measurement command, or in response to
  // a high-volume ASCII measurement command, the maximum is 75. The maximum is also
  // 75 in response to a continuous measurement command. Otherwise, the maximum is 35.
  int max_sdi_response = 76;
  // From SDI-12 Protocol v1.4, Table 11 The send data command (aD0!, aD1! . . . aD9!):
  // - the maximum number of digits for a data value is 7, even without a decimal point
  // - the minimum number of digits for a data value (excluding the decimal point) is 1
  // - the maximum number of characters in a data value is 9 (the (polarity sign + 7
  // digits + the decimal point))
  // - SRGD Note: The polarity symbol (+ or -) acts as a delimeter between the numeric
  // values
  int max_sdi_digits = 10;

  String compiled_response = "";

  bool success = true;

  // Create the return struct
  getResultsResult return_result;
  return_result.resultsReceived = 0;
  return_result.maxDataCommand  = 0;
  return_result.addressMatch    = true;
  return_result.crcMatch        = true;
  return_result.errorCode       = false;
  return_result.success         = true;

  while (resultsReceived < resultsExpected && cmd_number <= 9 && cmd_retries < 5) {
    bool    gotResults  = false;
    uint8_t cmd_results = 0;

    // Assemble the command based on how many commands we've already sent,
    // starting with D0 and ending with D9
    // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.clearBuffer();
    String command = "";
    command += address;
    command += "D";
    command += cmd_number;
    command += "!";
    mySDI12.sendCommand(command, wake_delay);
    delay(30);
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }
    char resp_buffer[max_sdi_response] = {'\0'};

    // read bytes into the char array until we get to a new line (\r\n)
    size_t bytes_read = mySDI12.readBytesUntil('\n', resp_buffer, max_sdi_response);
    // Serial.print(bytes_read);
    // Serial.println(" characters");

    size_t data_bytes_read = bytes_read - 1;  // subtract one for the /r before the /n
    String sdiResponse     = String(resp_buffer);
    compiled_response += sdiResponse;
    sdiResponse.trim();
    if (printIO) {
      Serial.print("<<<");
      Serial.println(sdiResponse);
      // Serial.println(sdiResponse.length());
      // Serial.print("<<<");
      // Serial.println(resp_buffer);
      // Serial.println(strnlen(resp_buffer, max_sdi_response));
    }
    // read and clear anything else from the buffer
    int extra_chars = 0;
    while (mySDI12.available()) {
      Serial.write(mySDI12.read());
      extra_chars++;
    }
    if (extra_chars > 0) {
      Serial.print(extra_chars);
      Serial.println(" additional characters received.");
    }
    mySDI12.clearBuffer();

    // check the crc, break if it's incorrect
    if (verify_crc) {
      bool crcMatch = mySDI12.verifyCRC(sdiResponse);
      // subtract the 3 characters of the CRC from the total number of data values
      data_bytes_read = data_bytes_read - 3;
      if (crcMatch) {
        if (printIO) { Serial.println("CRC valid"); }
      } else {
        if (printIO) { Serial.println("CRC check failed!"); }
        return_result.crcMatch = false;
        success                = false;
        // if we failed CRC in the response, add one to the retry
        // attempts but do not bump up the command number or transfer any
        // results because we want to retry the same data command to try get
        // a valid response
        cmd_retries++;
        // stop processing; no reason to read the numbers when we already know
        // something's wrong
        continue;
      }
    }

    // check the address, break if it's incorrect
    // NOTE: If the address is wrong because the response is garbled, the CRC check
    // above should fail. But we still verify the address in case we're not checking the
    // CRC or we got a well formed response from the wrong sensor.
    char returnedAddress = resp_buffer[0];
    if (returnedAddress != address) {
      if (printIO) {
        Serial.println("Wrong address returned!");
        Serial.print("Expected ");
        Serial.print(String(address));
        Serial.print(" Got ");
        Serial.println(String(returnedAddress));
        Serial.println(String(resp_buffer));
      }
      success                    = false;
      return_result.addressMatch = false;
      // if we didn't get the correct address, add one to the retry
      // attempts but do not bump up the command number or transfer any
      // results because we want to retry the same data command to try get
      // a valid response
      cmd_retries++;
      // stop processing; no reason to read the numbers when we already know
      // something's wrong
      continue;
    }

    bool    bad_read                     = false;
    char    float_buffer[max_sdi_digits] = {'\0'};
    bool    got_decimal                  = false;
    char*   dec_pl = float_buffer;  // just used for pretty printing
    uint8_t fb_pos = 0;             // start at start of buffer
    // iterate through the char array and to check results
    // NOTE: start at 1 since we already looked at the address!
    for (size_t i = 1; i <= data_bytes_read; i++) {
      // Get the character at position
      char c = resp_buffer[i];
      // Serial.print(i);
      // Serial.print(" of ");
      // Serial.print(data_bytes_read);
      // Serial.print(" '");
      // Serial.print(c);
      // Serial.println("'");
      // if we get a polarity sign (+ or -) that is not the first character after the
      // address, or we've reached the end of the buffer, then we're at the end of the
      // previous number and can parse the float buffer
      if ((((c == '-' || c == '+') && i != 1) || i == data_bytes_read) &&
          strnlen(float_buffer, max_sdi_digits) > 0) {
        float result = atof(float_buffer);
        if (printIO) {
          Serial.print("Result ");
          Serial.print(resultsReceived);
          Serial.print(", Raw value: ");
          Serial.print(float_buffer);
          dec_pl = strchr(float_buffer, '.');
          // NOTE: This bit below is just for pretty-printing
          size_t len_post_dec = 0;
          if (dec_pl != nullptr) { len_post_dec = strnlen(dec_pl, max_sdi_digits) - 1; }
          Serial.print(", Len after decimal: ");
          Serial.print(len_post_dec);
          Serial.print(", Parsed value: ");
          Serial.println(String(result, len_post_dec));
        }
        // add how many results we have
        if (result != -9999) {
          gotResults = true;
          resultsReceived++;
        }
        // check for a failure error code at the end
        if (error_result_number >= 1) {
          if (resultsReceived == error_result_number && result != no_error_value) {
            success                 = false;
            return_result.errorCode = true;
            if (printIO) {
              Serial.print("Got a failure code of ");
              Serial.println(String(result, strnlen(dec_pl, max_sdi_digits) - 1));
            }
          }
        }
        // empty the float buffer so it's ready for the next number
        float_buffer[0] = '\0';
        fb_pos          = 0;
        got_decimal     = false;
      }
      if (i == data_bytes_read) { continue; }
      // if we're mid-number and there's a digit, a decimal, or a negative sign in the
      // sdi12 response buffer, add it to the current float buffer
      if (c == '-' || (c >= '0' && c <= '9') || (c == '.' && !got_decimal)) {
        float_buffer[fb_pos] = c;
        fb_pos++;
        float_buffer[fb_pos] = '\0';  // null terminate the buffer
        // Serial.print("Added to float buffer, currently: '");
        // Serial.print(float_buffer);
        // Serial.print("' (");
        // Serial.print(strnlen(float_buffer, max_sdi_digits));
        // Serial.println(")");
      } else if (c == '+') {
        // if we get a "+", it's a valid SDI-12 polarity indicator, but not something
        // accepted by atof in parsing the float, so we just ignore it
        // NOTE: A mis-read like this should also cause the CRC to be wrong, but still
        // check here in case we're not using a CRC.
      } else {  //(c != '-' && c != '+' && (c < '0' || c > '9') && c != '.')
        Serial.print("Invalid data response character: ");
        Serial.write(c);
        Serial.println();
        bad_read = true;
      }
      // if we get a decimal, mark it so we can verify we don't get repeated decimals
      if (c == '.') { got_decimal = true; }
    }

    // if (!gotResults) {
    //   if (printIO) {
    //     Serial.println(("  No results received, will not continue requests!"));
    //   }
    //   break;
    // }  // don't do another loop if we got nothing

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

  if (printIO) {
    Serial.print("After ");
    Serial.print(cmd_number);
    Serial.print(" data commands got ");
    Serial.print(resultsReceived);
    Serial.print(" results of the expected ");
    Serial.print(resultsExpected);
    Serial.print(" expected. This is a ");
    Serial.println(resultsReceived == resultsExpected ? "success." : "failure.");
  }

  success &= resultsReceived == resultsExpected;
  this_result[charToDec(address) - firstAddress] = compiled_response;
  return_result.resultsReceived                  = resultsReceived;
  return_result.maxDataCommand                   = cmd_number;
  return_result.success                          = success;
  return return_result;
}

bool getContinuousResults(char address, int resultsExpected, bool printIO = true) {
  uint8_t resultsReceived = 0;
  uint8_t cmd_number      = 0;
  while (resultsReceived < resultsExpected && cmd_number <= 9) {
    String command = "";
    command += address;
    command += "R";
    command += cmd_number;
    command += "!";  // SDI-12 command to get data [address][D][dataOption][!]
    mySDI12.sendCommand(command, wake_delay);
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }

    uint32_t start = millis();
    while (mySDI12.available() < 3 && (millis() - start) < 1500) {};
    if (printIO) {
      Serial.print("<<<");
      Serial.write(mySDI12.read());  // ignore the repeated SDI12 address
    }

    while (mySDI12.available()) {
      char c = mySDI12.peek();
      if (c == '-' || c == '+' || (c >= '0' && c <= '9') || c == '.') {
        float result = mySDI12.parseFloat();
        Serial.print(String(result, 7));
        if (result != -9999) { resultsReceived++; }
      } else if (c >= 0 && c != '\r' && c != '\n') {
        Serial.write(mySDI12.read());
      } else {
        mySDI12.read();
      }
      delay(10);  // 1 character ~ 7.5ms
    }
    if (printIO) {
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

startMeasurementResult startMeasurement(char address, bool is_concurrent = false,
                                        bool request_crc = false, String meas_type = "",
                                        bool printIO = true) {
  // Create the return struct
  startMeasurementResult return_result;
  return_result.returnedAddress = "";
  return_result.meas_time_s     = 0;
  return_result.numberResults   = 0;

  String command = "";
  command += address;                    // All commands start with the address
  command += is_concurrent ? "C" : "M";  // C for concurrent, M for standard
  command += request_crc ? "C" : "";     // add an additional C to request a CRC
  command += meas_type;                  // Measurement type, "" or 0-9
  command += "!";                        // All commands end with "!"
  mySDI12.sendCommand(command, wake_delay);
  if (printIO) {
    Serial.print(">>>");
    Serial.println(command);
  }
  delay(30);

  // wait for acknowledgement with format [address][ttt (3 char, seconds)][number of
  // measurements available, 0-9]
  String sdiResponse = mySDI12.readStringUntil('\n');
  sdiResponse.trim();
  if (printIO) {
    Serial.print("<<<");
    Serial.println(sdiResponse);
  }
  mySDI12.clearBuffer();

  // check the address, return if it's incorrect
  String returnedAddress   = sdiResponse.substring(0, 1);
  char   ret_addr_array[2] = {'\0'};
  returnedAddress.toCharArray(ret_addr_array, sizeof(ret_addr_array));
  return_result.returnedAddress = ret_addr_array[0];
  if (returnedAddress != String(address)) {
    if (printIO) {
      Serial.println("Wrong address returned!");
      Serial.print("Expected ");
      Serial.print(String(address));
      Serial.print(" Got ");
      Serial.println(returnedAddress);
    }
    return return_result;
  }

  // find out how long we have to wait (in seconds).
  uint8_t meas_time_s       = sdiResponse.substring(1, 4).toInt();
  return_result.meas_time_s = meas_time_s;
  if (printIO) {
    Serial.print("expected measurement time: ");
    Serial.print(meas_time_s);
    Serial.print(" s, ");
  }

  // Set up the number of results to expect
  int numResults              = sdiResponse.substring(4).toInt();
  return_result.numberResults = numResults;
  if (printIO) {
    Serial.print("Number Results: ");
    Serial.println(numResults);
  }

  return return_result;
}

// This is a separate function in this example so the wait times can all be filled into
// the appropriate arrays
int startConcurrentMeasurement(char address, bool request_crc = false,
                               String meas_type = "", bool printIO = true) {
  startMeasurementResult startResult = startMeasurement(address, true, request_crc,
                                                        meas_type, printIO);

  uint8_t sensorNum        = charToDec(address - firstAddress);
  meas_time_ms[sensorNum]  = (static_cast<uint32_t>(startResult.meas_time_s)) * 1000;
  millisStarted[sensorNum] = millis();
  if (startResult.meas_time_s == 0) {
    millisReady[sensorNum] = millis();
  } else {
    // give an extra second
    // millisReady[sensorNum] = millis() + meas_time_ms[sensorNum] + 1000;
    // subtract a second to start polling early
    millisReady[sensorNum] = millis() + meas_time_ms[sensorNum] - 1000;
  }
  expectedResults[sensorNum] = startResult.numberResults;

  return startResult.numberResults;
}

uint32_t takeMeasurement(char address, bool request_crc = false, String meas_type = "",
                         bool printIO = true) {
  startMeasurementResult startResult = startMeasurement(address, false, request_crc,
                                                        meas_type, printIO);
  if (startResult.numberResults == 0) { return -1; }

  uint32_t timerStart = millis();
  uint32_t measTime   = -1;
  // wait up to 1 second longer than the specified return time
  while ((millis() - timerStart) <
         (static_cast<uint32_t>(startResult.meas_time_s) + 1) * 1000) {
    if (mySDI12.available()) {
      break;
    }  // sensor can interrupt us to let us know it is done early
  }
  measTime                  = millis() - timerStart;
  String interrupt_response = mySDI12.readStringUntil('\n');
  if (printIO) {
    Serial.print("<<<");
    Serial.println(interrupt_response);
    Serial.print("Completed after ");
    Serial.print(measTime);
    Serial.println(" ms");
  }

  // if we got results, return the measurement time, else -1
  if (getResults(address, startResult.numberResults, request_crc, printIO).success) {
    return measTime;
  }

  return -1;
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
bool checkActive(char address, int8_t numPings = 3, bool printIO = true) {
  mySDI12.clearBuffer();
  String command = "";
  command += (char)address;  // sends basic 'acknowledge' command [address][!]
  command += "!";

  for (int j = 0; j < numPings; j++) {  // goes through rapid contact attempts
    if (printIO) {
      Serial.print(">>>");
      Serial.println(command);
    }
    mySDI12.sendCommand(command, wake_delay);
    delay(30);

    // the sensor should just return its address followed by '\r\n'
    if (mySDI12.available()) {
      String sdiResponse_r = mySDI12.readStringUntil('\n');
      String sdiResponse   = sdiResponse_r;
      sdiResponse_r.replace("\r", "←");
      sdiResponse_r.replace("\n", "↓");
      if (printIO) {
        Serial.print("<<< '");
        Serial.print(sdiResponse_r);
        Serial.print("' (");
        Serial.print(sdiResponse_r.length());
        Serial.println(")");
      }
      sdiResponse.trim();
      sdiResponse.replace("\r", "←");
      sdiResponse.replace("\n", "↓");
      if (printIO) {
        Serial.print("<<< Trimmed: '");
        Serial.print(sdiResponse);
        Serial.print("' (");
        Serial.print(sdiResponse.length());
        Serial.println(")");
      }
      mySDI12.clearBuffer();

      // check the address, return false if it's incorrect
      String returnedAddress = sdiResponse.substring(0, 1);
      Serial.print("returnedAddress '");
      Serial.print(returnedAddress);
      Serial.print("' (");
      Serial.print(returnedAddress.length());
      Serial.println(")");
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

  // Fill arrays with 0's
  for (int8_t address_number = firstAddress; address_number <= lastAddress;
       address_number++) {
    int8_t pos           = address_number - firstAddress;
    isActive[pos]        = false;
    meas_time_ms[pos]    = 0;
    millisStarted[pos]   = 0;
    millisReady[pos]     = 0;
    expectedResults[pos] = 0;
    returnedResults[pos] = 0;
    prev_result[pos]     = "";
    this_result[pos]     = "";
  }

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

  // Quickly scan the address space
  Serial.println("\n\nScanning all addresses, please wait...");

  for (int8_t address_number = firstAddress; address_number <= lastAddress;
       address_number++) {
    int8_t pos     = address_number - firstAddress;
    char   address = decToChar(address_number);
    Serial.print("i: ");
    Serial.print(address_number);
    Serial.print(", address: ");
    Serial.print(address);
    Serial.print(", reversed: ");
    Serial.println(charToDec(address));
    if (checkActive(address, 5, true)) {
      numSensors++;
      isActive[pos] = 1;
      // Serial.println(", +");
      printInfo(address, true);
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
  Serial.println("Wait 10s...");
  delay(10000L);
  Serial.println();
  Serial.println("-------------------------------------------------------------------"
                 "------------");
}

void loop() {
  flip = !flip;  // flip the switch between concurrent and not
  Serial.print("Starting ");
  Serial.print(flip ? "concurrent" : "individual");
  // flip = 1;
  // flip           = 0;
  uint32_t start = millis();
  // Serial.print("Flip: ");
  // Serial.println(flip);

  // Fill arrays with 0's
  for (int8_t address_number = firstAddress; address_number <= lastAddress;
       address_number++) {
    int8_t pos           = address_number - firstAddress;
    meas_time_ms[pos]    = 0;
    millisStarted[pos]   = 0;
    millisReady[pos]     = 0;
    expectedResults[pos] = 0;
    returnedResults[pos] = 0;
  }

  if (flip) {
    // measure one at a time
    for (int8_t address_number = firstAddress; address_number <= lastAddress;
         address_number++) {
      int8_t pos     = address_number - firstAddress;
      char   address = decToChar(address_number);
      if (isActive[pos]) {
        for (uint8_t a = 0; a < commandsToTest; a++) {
          takeMeasurement(address, true, commands[a], true);
          Serial.println();
        }
        // getContinuousResults(address, 3);
      } else {
        Serial.print("Address ");
        Serial.print(address);
        Serial.println(" is not active");
      }
    }
    Serial.print("Total Time for Individual Measurements: ");
    Serial.println(millis() - start);
  } else {
    for (uint8_t a = 0; a < commandsToTest; a++) {
      uint32_t min_wait  = 60000L;
      uint32_t max_wait  = 0;
      uint32_t for_start = millis();
      // start all sensors measuring concurrently
      for (int8_t address_number = firstAddress; address_number <= lastAddress;
           address_number++) {
        int8_t pos     = address_number - firstAddress;
        char   address = decToChar(address_number);
        if (isActive[pos]) {
          startConcurrentMeasurement(address, true, commands[a], true);
          if (meas_time_ms[pos] < min_wait) { min_wait = meas_time_ms[pos]; }
          if (meas_time_ms[pos] > max_wait) { max_wait = meas_time_ms[pos]; }
        } else {
          Serial.print("Address ");
          Serial.print(address);
          Serial.println(" is not active");
        }
      }
      // min_wait = 800;
      min_wait = max(static_cast<uint32_t>(10), min_wait / 2);
      max_wait = max(static_cast<uint32_t>(1000),
                     max_wait + static_cast<uint32_t>(2000));
      Serial.print("minimum expected wait for all sensors: ");
      Serial.println(min_wait);
      Serial.print("maximum expected wait for all sensors: ");
      Serial.println(max_wait);

#if defined(TEST_PRINT_ARRAY)
      Serial.print("address_number,\t");
      Serial.print("address,\t");
      Serial.print("position,\t");
      Serial.print("isActive[i],\t");
      Serial.print("millis,\t");
      Serial.print("timeWaited,\t");
      Serial.print("millisReady[i],\t");
      Serial.print("expectedResults[i],\t");
      Serial.print("returnedResults[i],\t");
      Serial.print("millis() > millisReady[i],\t");
      Serial.print("expectedResults[i] > 0,\t");
      Serial.print("returnedResults[i] < expectedResults[i],\t");
      Serial.print("numSensors,\t");
      Serial.print("numReadingsRecorded,\t");
      Serial.print("maxDataCommand,\t");
      Serial.print("resultsReceived,\t");
      Serial.print("errorCode,\t");
      Serial.print("crcMatch,\t");
      Serial.print("gotGoodResults,\t");
      Serial.print("numReadingsRecorded");
      Serial.println();
#endif

      uint8_t numReadingsRecorded = 0;
      delay(min_wait);

      do {
        // get all readings
        for (int8_t address_number = firstAddress; address_number <= lastAddress;
             address_number++) {
          int8_t   pos        = address_number - firstAddress;
          uint32_t timeWaited = 0;
          if (millisStarted[pos] != 0) { timeWaited = millis() - millisStarted[pos]; }
          if (this_result[pos] != "") { prev_result[pos] = this_result[pos]; }

          char addr = decToChar(address_number);

#if defined(TEST_PRINT_ARRAY)
          Serial.print(address_number);
          Serial.print(",\t\"");
          Serial.print(decToChar(address_number));
          Serial.print("\",\t");
          Serial.print(pos);
          Serial.print(",\t\"");
          Serial.print(isActive[pos]);
          Serial.print(",\t");
          Serial.print(millis());
          Serial.print(",\t");
          Serial.print(timeWaited);
          Serial.print(",\t");
          Serial.print(millisReady[pos]);
          Serial.print(",\t");
          Serial.print(expectedResults[pos]);
          Serial.print(",\t");
          Serial.print(returnedResults[pos]);
          Serial.print(",\t");
          Serial.print(millis() > millisReady[pos]);
          Serial.print(",\t");
          Serial.print(expectedResults[pos] > 0);
          Serial.print(",\t");
          Serial.print(returnedResults[pos] < expectedResults[pos]);
          Serial.print(",\t");
          Serial.print(numSensors);
          Serial.print(",\t");
          Serial.print(numReadingsRecorded);
#endif

          if (isActive[pos] && (millis() > millisReady[pos]) &&
              (expectedResults[pos] > 0) &&
              (returnedResults[pos] < expectedResults[pos])) {
#ifndef TEST_PRINT_ARRAY
            Serial.print("timeWaited: ");
            Serial.println(timeWaited);
#endif
            getResultsResult cResult = getResults(addr, expectedResults[pos], true);
            returnedResults[pos]     = cResult.resultsReceived;
            bool gotGoodResults      = cResult.success;
            if (gotGoodResults) {
              numReadingsRecorded++;
#ifndef TEST_PRINT_ARRAY
              Serial.print("Got results from ");
              Serial.print(numReadingsRecorded);
              Serial.print(" of ");
              Serial.print(numSensors);
              Serial.print(" sensors");
#endif
            }
#if defined(TEST_PRINT_ARRAY)
            Serial.print(",\t");
            Serial.print(cResult.maxDataCommand);
            Serial.print(",\t");
            Serial.print(cResult.resultsReceived);
            Serial.print(",\t");
            Serial.print(cResult.errorCode);
            Serial.print(",\t");
            Serial.print(cResult.crcMatch);
            Serial.print(",\t");
            Serial.print(gotGoodResults);
#endif
          }
        }

      } while (millis() - for_start < max_wait && numReadingsRecorded < numSensors);
    }
    Serial.print("Total Time for Concurrent Measurements: ");
    Serial.println(millis() - start);
  }

  Serial.println("-------------------------------------------------------------------"
                 "------------");
}
