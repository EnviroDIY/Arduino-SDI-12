/**
 * @file g_terminal_window.ino
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 *            This example is published under the BSD-3 license.
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 * @date August 2013
 * @author Ruben Kertesz <github@emnet.net> or @rinnamon on twitter
 * @date 2016
 *
 * @brief Example G: Using the Arduino as a Command Terminal for SDI-12 Sensors
 *
 * This is a simple demonstration of the SDI-12 library for Arduino.  It's purpose is to
 * allow a user to interact with an SDI-12 sensor directly, issuing commands through a
 * serial terminal window.
 *
 * Edited by Ruben Kertesz for ISCO Nile 502 2/10/2016
 */

#include <SDI12.h>

#define SERIAL_BAUD 115200 /*!< The baud rate for the output serial port */
#define DATA_PIN 7         /*!< The pin of the SDI-12 data bus */
#define POWER_PIN 22       /*!< The sensor power pin (or -1 if not switching power) */

/** Define the SDI-12 bus */
SDI12 mySDI12(DATA_PIN);

char   inByte      = 0;
String sdiResponse = "";
String myCommand   = "";

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
}

void loop() {
  if (Serial.available()) {
    inByte = Serial.read();
    if ((inByte != '\n') &&
        (inByte != '\r')) {  // read all values entered in terminal window before enter
      myCommand += inByte;
      delay(10);  // 1 character ~ 7.5ms
    }
  }

  if (inByte == '\r') {  // once we press enter, send string to SDI sensor/probe
    inByte = 0;
    Serial.println(myCommand);
    mySDI12.sendCommand(myCommand);
    delay(30);  // wait a while for a response

    while (mySDI12.available()) {  // build a string of the response
      char c = mySDI12.read();
      if ((c != '\n') && (c != '\r')) {
        sdiResponse += c;
        delay(10);  // 1 character ~ 7.5ms
      }
    }
    if (sdiResponse.length() >= 1)
      Serial.println(sdiResponse);  // write the response to the screen

    mySDI12.clearBuffer();  // clear the line
    myCommand   = "";
    sdiResponse = "";
  }
}
