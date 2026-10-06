/**
 * @example{lineno} TestSensorTiming.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 * @date March 2024
 */

#include <SDI12.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

/* connection information */
uint32_t serialBaud    = 115200; /*!< The baud rate for the output serial port */
int8_t   dataPin       = SDI12_DATA_PIN;  /*!< The pin of the SDI-12 data bus */
int8_t   powerPin      = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
char     sensorAddress = '0';             /*!< The address of the SDI-12 sensor */

/** Define the SDI-12 bus */
SDI12 mySDI12(dataPin);

/** Define some testing specs */

/** Error codes, if returned */
int8_t error_result_number = 7;
float  no_error_value      = 0;

/** Testing turning off power */
bool    testPowerOff    = false;
int32_t min_power_delay = 100L;       /*!< The min time to test wake after power on. */
int32_t max_power_delay = 180000L;    /*!< The max time to test wake after power on. */
int32_t increment_power_delay = 100L; /*!< The time to lengthen waits between reps. */
int32_t power_off_time        = 60000L; /*!< The time to power off between tests. */
/** NOTE: set the power off time to be something similar to what you will be using the
 * the real world! Some sensors take longer to warm up if they've been off for a while.
 */

/** Testing the length of the wait after the break */
bool    testWakeDelay  = true;
int32_t min_wake_delay = 0;   /*!< The min time to test wake after a line break. */
int32_t max_wake_delay = 100; /*!< The max time to test wake (should be <=100). */
int32_t increment_wake = 5;   /*!< The time to lengthen waits between reps. */

/** set some initial values */
int32_t power_delay = min_power_delay;
int32_t wake_delay  = min_wake_delay;

uint32_t total_meas_time = 0;
uint32_t total_meas_made = 0;
uint32_t max_meas_time   = 0;

struct startMeasurementResult {  // Structure declaration
  String  returned_address;
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

    // check the crc, continue to the next attempt if it's incorrect
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

    // check the address, continue to the next attempt if it's incorrect
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
  return_result.resultsReceived = resultsReceived;
  return_result.maxDataCommand  = cmd_number;
  return_result.success         = success;
  return return_result;
}

startMeasurementResult startMeasurement(char address, bool is_concurrent = false,
                                        bool request_crc = false, String meas_type = "",
                                        bool printIO = true) {
  // Create the return struct
  startMeasurementResult return_result;
  return_result.returned_address = "";
  return_result.meas_time_s      = 0;
  return_result.numberResults    = 0;

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
  return_result.returned_address = ret_addr_array[0];
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
    // wait for a response; the sensor must return within 100ms
    uint32_t waitStart = millis();
    while ((millis() - waitStart) < 150 && !mySDI12.available()) { yield(); }

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

  if (testPowerOff && testWakeDelay) {
    Serial.println(
      "Both wake delay and power off testing cannot be enabled at the same time!");
    Serial.println("Pick one to test and reprogram the logger");
    while (1);
  }

  // Power the sensors;
  if (powerPin >= 0 && !testPowerOff) {
    Serial.print("Powering up sensors with pin ");
    Serial.print(String(powerPin));
    Serial.println(", wait 30s...");
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(30000L);
  } else {
    Serial.println("Wait 5s...");
    delay(5000L);
  }
}

void loop() {
  bool got_good_results = true;
  int  checks_at_time   = 0;
  while (got_good_results && checks_at_time < 25) {
    Serial.println("\n\n------------------------");
    Serial.print("Repeat attempt ");
    Serial.print(checks_at_time);
    Serial.print(" with power on warm-up time of ");
    Serial.print(power_delay);
    Serial.print(" and after-break wake up delay of ");
    Serial.print(wake_delay);
    Serial.print(" and a stream timeout of ");
    Serial.println(mySDI12.getTimeout());

    // Power down the sensors;
    if (powerPin >= 0 && testPowerOff) {
      Serial.print("Powering down sensors, wait ");
      Serial.print(power_off_time);
      Serial.println(" ms");
      pinMode(powerPin, OUTPUT);
      digitalWrite(powerPin, LOW);
      delay(power_off_time);
    }

    // Power up the sensors;
    if (powerPin >= 0 && testPowerOff) {
      Serial.print("Powering up sensors, wait ");
      Serial.print(power_delay);
      Serial.println(" ms");
      pinMode(powerPin, OUTPUT);
      digitalWrite(powerPin, HIGH);
      delay(power_delay);
      mySDI12.clearBuffer();
    }

    Serial.println("------------------------");
    Serial.println("Checking if sensor is active...");
    bool isActive = checkActive(sensorAddress, 3, true);
    got_good_results &= isActive;

    Serial.println("------------------------");
    Serial.println("Testing if the sensor returns valid info...");
    bool gotInfo = printInfo(sensorAddress, true);
    got_good_results &= gotInfo;

    Serial.println("------------------------");
    Serial.println("Testing if a measurement returns the right number of values in the "
                   "expected time...");
    uint32_t this_meas_time   = takeMeasurement(sensorAddress, true, "", true);
    bool     this_result_good = this_meas_time != static_cast<uint32_t>(-1);

    if (this_result_good) {
      total_meas_time += this_meas_time;
      total_meas_made++;
      if (this_meas_time > max_meas_time) { max_meas_time = this_meas_time; }
      Serial.print("Warm-up Time: ");
      Serial.print(power_delay);
      Serial.print(", This measurement time: ");
      Serial.print(this_meas_time);
      Serial.print(", Mean measurement time: ");
      Serial.print(total_meas_time / total_meas_made);
      Serial.print(", Max measurement time: ");
      Serial.println(max_meas_time);
    }

    got_good_results &= this_result_good;
    checks_at_time++;

    if (!got_good_results) {
      Serial.print("Time check failed after ");
      Serial.print(checks_at_time);
      Serial.println(" reps");
      total_meas_time = 0;
      total_meas_made = 0;
      max_meas_time   = 0;
    }
  }

  // if we got a good result 25x at this warm-up, keep testing how long the
  // measurements take

  Serial.println("\n------------------------");
  Serial.println("Continuing to test measurement times...");
  while (got_good_results) {
    uint32_t this_meas_time = takeMeasurement(sensorAddress, true, "", false);
    if (this_meas_time != static_cast<uint32_t>(-1)) {
      total_meas_time += this_meas_time;
      total_meas_made++;
      if (this_meas_time > max_meas_time) { max_meas_time = this_meas_time; }
      Serial.print("Warm-up Time: ");
      Serial.print(power_delay);
      Serial.print(", This measurement time: ");
      Serial.print(this_meas_time);
      Serial.print(", Mean measurement time: ");
      Serial.print(total_meas_time / total_meas_made);
      Serial.print(", Max measurement time: ");
      Serial.println(max_meas_time);
    }
  }

  Serial.println("-------------------------------------------------------------------"
                 "------------");
  if (testPowerOff) {
    if (power_delay > max_power_delay) {
      power_delay = min_power_delay;
    } else {
      power_delay = power_delay + increment_power_delay;
    }
  }
  if (testWakeDelay) {
    if (wake_delay > max_wake_delay) {
      wake_delay = min_wake_delay;
    } else {
      wake_delay = wake_delay + increment_wake;
    }
  }
}
