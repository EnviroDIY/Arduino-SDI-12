/**
 * @file d_simple_logger.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Sara Damiano <sdamiano@stroudcenter.org>
 * @date March 2021
 */

#include <SDI12.h>

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define SENSOR_ADDRESS '0' /*!< The address of the SDI-12 sensor */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */

/** Define the SDI-12 bus */
SDI12   mySDI12(DATA_PIN);
int32_t wake_delay      = 0;  /*!< The time for the board to wake after a line break. */
int32_t increment_wake  = 10; /*!< The time to lengthen waits between reps. */
int32_t power_delay     = 5400;   /*!< The time for the board to wake after power on. */
int32_t increment_power = 50;    /*!< The time to lengthen waits between reps. */
int32_t max_power_delay = 10000L; /*!< The max time to test wake after power on. */

/**
 * @brief gets identification information from a sensor, and prints it to the serial
 * port
 *
 * @param i a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
 */
bool printInfo(char i, bool printCommands = true) {
  String command = "";
  command += (char)i;
  command += "I!";
  mySDI12.sendCommand(command, wake_delay);
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

  if (sdiResponse.length() < 3) { return false; };
  return true;
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
    mySDI12.sendCommand(command, wake_delay);
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
}

void loop() {
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
    delay(power_delay);
  }

  if (checkActive(SENSOR_ADDRESS, 5, true)) {
    Serial.print("Got response after ");
    Serial.print(power_delay);
    Serial.print("ms after power with ");
    Serial.print(wake_delay);
    Serial.println("ms with wake delay");
    if (printInfo(SENSOR_ADDRESS, true)) {
      // if we got sensor info, stop
      while (1)
        ;
    }
  } else {
    Serial.print("No response after ");
    Serial.print(power_delay);
    Serial.print("ms after power with ");
    Serial.print(wake_delay);
    Serial.println("ms with wake delay");
  }
  Serial.println("-------------------------------------------------------------------"
                 "------------");
  if (power_delay > max_power_delay) {
    power_delay = 0;
    wake_delay  = wake_delay + increment_wake;
  } else {
    power_delay = power_delay + increment_power;
  }
}
