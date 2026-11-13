/**
 * @example{lineno} TestRecorder.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 *
 * @brief Compile and hardware exercise sketch for every public blocking recorder
 * operation.
 */

#include <SDI12Config.h>
#if (defined(__AVR__) || defined(ARDUINO_ARCH_AVR)) && !defined(SDI12_INTERNAL_PCINT)
#include <EnableInterrupt.h>
#endif

#include <SDI12Recorder.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif
#ifndef SDI12_TERMINAL_BAUD
#define SDI12_TERMINAL_BAUD 115200
#endif

// #define TEST_PRINT_ARRAY

/* connection information */
#if F_CPU > 48000000L && !defined(SDI12_TERMINAL_BAUD)
uint32_t serialBaud = 921600; /*!< The baud rate for the serial port */
#elif !defined(SDI12_TERMINAL_BAUD)
uint32_t serialBaud = 115200; /*!< The baud rate for the serial port */
#else
uint32_t serialBaud = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
#endif
int8_t       dataPin  = SDI12_DATA_PIN;  /*!< The pin of the SDI-12 data bus */
int8_t       powerPin = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */
const int8_t firstAddress =
  0; /* The first address in the address space to check (0='0') */
const int8_t lastAddress =
  7; /* The last address in the address space to check (61='z') */
const int8_t commandsToTest =
  1; /*!< The number of measurement commands to test, between 1 and 11. */

/** Create a new SDI12 data layer and recorder object */
SDI12         mySDI12(dataPin);
SDI12Recorder mySDI12Recorder(mySDI12);

/** Define some testing specs */
/// The array of commands to test
String commands[] = {"", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
/// variable that alternates output type back and forth between parsed and raw
boolean flip = 0;
/** The number of sensors found on the bus */
uint8_t numSensors = 0;

const int8_t n_addresses = (lastAddress - firstAddress) + 1;

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

String prev_result[n_addresses];
String this_result[n_addresses];

/** Error codes, if returned */
int8_t error_result_number = 7;
float  no_error_value      = 0;

struct startMeasurementResult {  // Structure declaration
  String   returned_address;
  uint16_t meas_time_s;
  int      numberResults;
};

struct getResultsResult {  // Structure declaration
  uint8_t resultsReceived;
  uint8_t maxDataCommand;
  bool    addressMatch;
  bool    crcMatch;
  bool    errorCode;
  bool    success;
};

float recorderValues[99];

/**
 * @brief gets identification information from a sensor, and pretty-prints it to the
 * serial port
 *
 * @param address a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
void printInfo(char address) {
  SDI12Recorder::Identification sensorIdStruct;
  bool idSuccess = mySDI12Recorder.identify(address, sensorIdStruct);

  if (idSuccess) {
    mySDI12Recorder.printSensorInfo(sensorIdStruct, Serial);
  } else {
    Serial.print(F("Address "));
    Serial.print(address);
    Serial.print(F(": "));
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }
}

getResultsResult getResults(char address, int resultsExpected, bool verify_crc = false,
                            bool printIO = true) {
  (void)verify_crc;

  getResultsResult return_result = {};
  return_result.addressMatch     = true;
  return_result.crcMatch         = true;

  if (resultsExpected < 0 || resultsExpected > 99) {
    return_result.success = false;
    return return_result;
  }

  mySDI12Recorder.requestData(address, recorderValues, 99U);
  while (mySDI12Recorder.isBusy()) {
    mySDI12Recorder.update();
    yield();
  }

  return_result.resultsReceived = mySDI12Recorder.dataValueCount();
  return_result.addressMatch    = mySDI12Recorder.result() !=
    SDI12Recorder::RESULT_WRONG_ADDRESS;
  return_result.crcMatch = mySDI12Recorder.result() != SDI12Recorder::RESULT_CRC_ERROR;

  for (uint8_t i = 0U; i < return_result.resultsReceived; ++i) {
    if (printIO) {
      Serial.print(F("Result "));
      Serial.print(i);
      Serial.print(F(": "));
    }
    Serial.print(String(recorderValues[i], 7));
    if (printIO) {
      Serial.println();
    } else {
      Serial.print(F(", "));
    }

    if (error_result_number >= 1 &&
        i + 1U == static_cast<uint8_t>(error_result_number) &&
        recorderValues[i] != no_error_value) {
      return_result.errorCode = true;
    }
  }

  return_result.success = mySDI12Recorder.result() == SDI12Recorder::RESULT_SUCCESS &&
    return_result.resultsReceived == resultsExpected && !return_result.errorCode;
  if (printIO && !return_result.success) {
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }
  return return_result;
}

bool getContinuousResults(char address, int resultsExpected, bool printIO = true) {
  uint8_t resultsReceived = 0;
  uint8_t dataOption      = 0U;
  while (resultsReceived < resultsExpected && dataOption <= 9U) {
    const size_t valuesReceived = mySDI12Recorder.requestContinuousData(
      address, recorderValues, 99U, dataOption, false);
    if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
      if (printIO) {
        mySDI12Recorder.printRecorderResult(Serial);
        Serial.println();
      }
      break;
    }

    for (size_t i = 0U; i < valuesReceived; ++i) {
      Serial.print(String(recorderValues[i], 7));
      Serial.print(F(", "));
    }
    resultsReceived += static_cast<uint8_t>(valuesReceived);
    if (printIO) {
      Serial.print(F("Total Results Received: "));
      Serial.print(resultsReceived);
      Serial.print(F(", Remaining: "));
      Serial.println(resultsExpected - resultsReceived);
    }
    if (valuesReceived == 0U) { break; }
    ++dataOption;
  }

  return resultsReceived == resultsExpected;
}

/**
 * @brief Exercise every beginner-facing blocking recorder command.
 *
 * Define TEST_BEGINNER_COMMANDS to run this hardware test from setup(). Address query
 * requires exactly one attached sensor, and the address-change test temporarily moves
 * that sensor to @p newAddress before restoring its original address.
 */
void testBeginnerCommands(char address, char newAddress) {
  float values[9];

  const char queriedAddress = mySDI12Recorder.queryAddress();
  Serial.print(F("Queried address: "));
  Serial.println(queriedAddress);

  mySDI12Recorder.requestAcknowledgement(address);

  SDI12Recorder::Identification identification;
  mySDI12Recorder.identify(address, identification);

  if (mySDI12Recorder.changeAddress(address, newAddress)) {
    if (!mySDI12Recorder.changeAddress(newAddress, address)) {
      Serial.print(F("Could not restore the sensor address; address "));
      Serial.print(newAddress);
      Serial.println(F(" may still be active."));
    }
  }

  mySDI12Recorder.measure(address, values, 9U, 0U, false);
  mySDI12Recorder.measure(address, values, 9U, 0U, true);
  mySDI12Recorder.requestContinuousData(address, values, 9U, 0U, false);
  mySDI12Recorder.requestContinuousData(address, values, 9U, 0U, true);
}

startMeasurementResult startMeasurement(char address, bool is_concurrent = false,
                                        bool request_crc = false, String meas_type = "",
                                        bool printIO = true) {
  // Create the return struct
  startMeasurementResult return_result;
  return_result.returned_address = "";
  return_result.meas_time_s      = 0;
  return_result.numberResults    = 0;

  uint8_t measurementNumber = 0U;
  if (meas_type.length() == 1U && meas_type[0] >= '0' && meas_type[0] <= '9') {
    measurementNumber = static_cast<uint8_t>(meas_type[0] - '0');
  } else if (meas_type.length() > 0U) {
    measurementNumber = 10U;
  }

  if (is_concurrent) {
    mySDI12Recorder.startConcurrentMeasurement(address, measurementNumber, request_crc);
  } else {
    mySDI12Recorder.startMeasurement(address, measurementNumber, request_crc);
  }
  while (mySDI12Recorder.isBusy()) {
    mySDI12Recorder.update();
    yield();
  }

  if (printIO) {
    Serial.print(is_concurrent ? F("Concurrent measurement result: ")
                               : F("Normal measurement result: "));
    mySDI12Recorder.printRecorderResult(Serial);
    Serial.println();
  }
  if (mySDI12Recorder.result() != SDI12Recorder::RESULT_SUCCESS) {
    return return_result;
  }

  return_result.returned_address = String(address);
  return_result.meas_time_s      = mySDI12Recorder.measurementSeconds();
  return_result.numberResults    = mySDI12Recorder.expectedValueCount();
  return return_result;
}

// This is a separate function in this example so the wait times can all be filled into
// the appropriate arrays
int startConcurrentMeasurement(char address, bool request_crc = false,
                               String meas_type = "", bool printIO = true) {
  startMeasurementResult startResult = startMeasurement(address, true, request_crc,
                                                        meas_type, printIO);

  uint8_t sensorNum =
    static_cast<uint8_t>(SDI12Address::toIndex(address) - firstAddress);
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
    Serial.print(F("<<<"));
    Serial.println(interrupt_response);
    Serial.print(F("Completed after "));
    Serial.print(measTime);
    Serial.println(F(" ms"));
  }

  // if we got results, return the measurement time, else -1
  if (getResults(address, startResult.numberResults, request_crc, printIO).success) {
    return measTime;
  }

  return -1;
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);
  char tbuf[3] = {'\0'};

  Serial.print(F("Opening SDI-12 bus on pin "));
  Serial.print(dataPin);
  Serial.print(F(" ("));
  Serial.print(itoa(dataPin, tbuf, 10));
  Serial.print(')');
  Serial.println(F("..."));
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println(F("Timeout value: "));
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
    Serial.print(F("Powering up sensors with pin "));
    Serial.print(String(powerPin));
    Serial.println(F(", wait 10s..."));
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(10000L);
  } else {
    Serial.println(F("Wait 5s..."));
    delay(5000L);
  }

#if defined(TEST_BEGINNER_COMMANDS)
  testBeginnerCommands('0', '1');
#endif

  // Quickly scan the address space
  Serial.println(F("\n\nScanning all addresses, please wait..."));

  for (int8_t address_number = firstAddress; address_number <= lastAddress;
       address_number++) {
    int8_t pos     = address_number - firstAddress;
    char   address = SDI12Address::fromIndex(address_number);
    Serial.print(F("i: "));
    Serial.print(address_number);
    Serial.print(F(", address: "));
    Serial.print(address);
    Serial.print(F(", reversed: "));
    Serial.println(static_cast<int>(SDI12Address::toIndex(address)));
    if (mySDI12Recorder.requestAcknowledgement(address)) {
      numSensors++;
      isActive[pos] = 1;
      printInfo(address);
    } else {
      Serial.print(F("Address "));
      Serial.print(address);
      Serial.print(F(": "));
      mySDI12Recorder.printRecorderResult(Serial);
      Serial.println();
    }
  }
  Serial.print(F("Total number of sensors found:  "));
  Serial.println(numSensors);

  if (numSensors == 0) {
    Serial.println(
      F("No sensors found, please check connections and restart the Arduino."));
    while (true);  // do nothing forever
  }

  Serial.println();
  Serial.println(F(
    "-------------------------------------------------------------------------------"));
  Serial.println(F("Wait 10s..."));
  delay(10000L);
  Serial.println();
  Serial.println(F(
    "-------------------------------------------------------------------------------"));
}

void loop() {
  flip = !flip;  // flip the switch between concurrent and not
  Serial.print(F("\nStarting "));
  Serial.print(flip ? "concurrent" : "individual");
  // flip = 1;
  // flip           = 0;
  uint32_t start = millis();
  // Serial.print(F("Flip: "));
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
      char   address = SDI12Address::fromIndex(address_number);
      if (isActive[pos]) {
        for (uint8_t a = 0; a < commandsToTest; a++) {
          takeMeasurement(address, true, commands[a], true);
          Serial.println();
        }
        // getContinuousResults(address, 3);
      } else {
        Serial.print(F("Address "));
        Serial.print(address);
        Serial.println(F(" is not active"));
      }
    }
    Serial.print(F("Total Time for Individual Measurements: "));
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
        char   address = SDI12Address::fromIndex(address_number);
        if (isActive[pos]) {
          startConcurrentMeasurement(address, true, commands[a], true);
          if (meas_time_ms[pos] < min_wait) { min_wait = meas_time_ms[pos]; }
          if (meas_time_ms[pos] > max_wait) { max_wait = meas_time_ms[pos]; }
        } else {
          Serial.print(F("Address "));
          Serial.print(address);
          Serial.println(F(" is not active"));
        }
      }
      // min_wait = 800;
      min_wait = max(static_cast<uint32_t>(10), min_wait / 2);
      max_wait = max(static_cast<uint32_t>(1000),
                     max_wait + static_cast<uint32_t>(2000));
      Serial.print(F("minimum expected wait for all sensors: "));
      Serial.println(min_wait);
      Serial.print(F("maximum expected wait for all sensors: "));
      Serial.println(max_wait);

#if defined(TEST_PRINT_ARRAY)
      Serial.print(F("address_number,\t"));
      Serial.print(F("address,\t"));
      Serial.print(F("position,\t"));
      Serial.print(F("isActive[i],\t"));
      Serial.print(F("millis,\t"));
      Serial.print(F("timeWaited,\t"));
      Serial.print(F("millisReady[i],\t"));
      Serial.print(F("expectedResults[i],\t"));
      Serial.print(F("returnedResults[i],\t"));
      Serial.print(F("measurementState[i],\t"));
      Serial.print(F("expectedResults[i] > 0,\t"));
      Serial.print(F("returnedResults[i] < expectedResults[i],\t"));
      Serial.print(F("numSensors,\t"));
      Serial.print(F("numReadingsRecorded,\t"));
      Serial.print(F("maxDataCommand,\t"));
      Serial.print(F("resultsReceived,\t"));
      Serial.print(F("errorCode,\t"));
      Serial.print(F("crcMatch,\t"));
      Serial.print(F("gotGoodResults,\t"));
      Serial.print(F("numReadingsRecorded"));
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

          char addr = SDI12Address::fromIndex(address_number);

#if defined(TEST_PRINT_ARRAY)
          const SDI12Recorder::MeasurementState sensorMeasurementState =
            mySDI12Recorder.measurementState(addr);
          Serial.print(address_number);
          Serial.print(F(",\t"));
          Serial.print(SDI12Address::fromIndex(address_number));
          Serial.print(F(",\t"));
          Serial.print(pos);
          Serial.print(F(",\t"));
          Serial.print(isActive[pos]);
          Serial.print(F(",\t"));
          Serial.print(millis());
          Serial.print(F(",\t"));
          Serial.print(timeWaited);
          Serial.print(F(",\t"));
          Serial.print(millisReady[pos]);
          Serial.print(F(",\t"));
          Serial.print(expectedResults[pos]);
          Serial.print(F(",\t"));
          Serial.print(returnedResults[pos]);
          Serial.print(F(",\t"));
          Serial.print(static_cast<uint8_t>(sensorMeasurementState));
          Serial.print(F(",\t"));
          Serial.print(expectedResults[pos] > 0);
          Serial.print(F(",\t"));
          Serial.print(returnedResults[pos] < expectedResults[pos]);
          Serial.print(F(",\t"));
          Serial.print(numSensors);
          Serial.print(F(",\t"));
          Serial.print(numReadingsRecorded);
#endif

          if (isActive[pos] && mySDI12Recorder.isMeasurementReady(addr) &&
              (expectedResults[pos] > 0) &&
              (returnedResults[pos] < expectedResults[pos])) {
#ifndef TEST_PRINT_ARRAY
            Serial.print(F("timeWaited: "));
            Serial.println(timeWaited);
#endif
            getResultsResult cResult = getResults(addr, expectedResults[pos], true);
            returnedResults[pos]     = cResult.resultsReceived;
            bool gotGoodResults      = cResult.success;
            if (gotGoodResults) {
              numReadingsRecorded++;
#ifndef TEST_PRINT_ARRAY
              Serial.print(F("Got results from "));
              Serial.print(numReadingsRecorded);
              Serial.print(F(" of "));
              Serial.print(numSensors);
              Serial.print(F(" sensors"));
#endif
            }
#if defined(TEST_PRINT_ARRAY)
            Serial.print(F(",\t"));
            Serial.print(cResult.maxDataCommand);
            Serial.print(F(",\t"));
            Serial.print(cResult.resultsReceived);
            Serial.print(F(",\t"));
            Serial.print(cResult.errorCode);
            Serial.print(F(",\t"));
            Serial.print(cResult.crcMatch);
            Serial.print(F(",\t"));
            Serial.print(gotGoodResults);
#endif
          }
        }

      } while (millis() - for_start < max_wait && numReadingsRecorded < numSensors);
    }
    Serial.print(F("Total Time for Concurrent Measurements: "));
    Serial.println(millis() - start);
  }
  Serial.println(F("-------------------------------------------------------------------"
                   "------------"));
}
