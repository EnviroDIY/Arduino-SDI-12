/**
 * @example{lineno} TestWarmUp.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 * @date March 2021
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
char     sensorAddress = '0';             /*!< The address of the SDI-12 sensor */
int8_t   powerPin      = SDI12_POWER_PIN; /*!< The sensor power pin (or -1) */

/** Define the SDI-12 bus */
SDI12 mySDI12(dataPin);

/** Define some testing specs */

/** Error codes, if returned */
int8_t error_result_number = 7;
float  no_error_value      = 0;

/** Testing turning off power */
int32_t min_power_delay = 100L;   /*!< The min time to test wake after power on. */
int32_t max_power_delay = 10000L; /*!< The max time to test wake after power on. */
int32_t increment_power = 100;    /*!< The time to lengthen waits between reps. */

/** Testing the length of the break */
int32_t min_wake_delay = 0;   /*!< The min time to test wake after a line break. */
int32_t max_wake_delay = 100; /*!< The max time to test wake (should be <=100). */
int32_t increment_wake = 5;   /*!< The time to lengthen waits between reps. */

/** set some initial values */
int32_t power_delay = min_power_delay;
int32_t wake_delay  = min_wake_delay;

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

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L);

  Serial.print("Opening SDI-12 bus on pin ");
  Serial.print(dataPin);
  Serial.println("...");
  mySDI12.begin();
  delay(500);  // allow things to settle

  Serial.println("Timeout value: ");
  Serial.println(mySDI12.TIMEOUT);
}

void loop() {
  while (wake_delay <= max_wake_delay) {
    Serial.println("-------------------------------------------------------------------"
                   "------------");
    // Power the sensors;
    if (powerPin >= 0) {
      Serial.println("Powering down sensors...");
      pinMode(powerPin, OUTPUT);
      digitalWrite(powerPin, LOW);
      delay(5000L);
    }

    // Power the sensors;
    if (powerPin >= 0) {
      Serial.println("Powering up sensors...");
      pinMode(powerPin, OUTPUT);
      digitalWrite(powerPin, HIGH);
      delay(power_delay);
      mySDI12.clearBuffer();
    }

    if (checkActive(sensorAddress, 1, true)) {
      Serial.print("Got some response after ");
      Serial.print(power_delay);
      Serial.print("ms after power with ");
      Serial.print(wake_delay);
      Serial.println("ms with wake delay");
      if (printInfo(sensorAddress, true)) {
        // if we got sensor info, stop
        Serial.println("Looks good.  Stopping.");
        while (1);
      } else {
        Serial.println("Sensor info not valid!");
      }
    } else {
      Serial.print("No response after ");
      Serial.print(power_delay);
      Serial.print("ms after power with ");
      Serial.print(wake_delay);
      Serial.println("ms with wake delay");
    }

    Serial.print("Increasing the power delay by ");
    Serial.print(increment_power);
    Serial.println("ms");
    power_delay += increment_power;
    Serial.print("The next delay will be ");
    Serial.print(power_delay);
    Serial.println("ms");
    if (power_delay > max_power_delay) {
      Serial.println("Reached maximum power delay!");
      Serial.println("FINISHED!!");
      while (1);
    }
  }
}
