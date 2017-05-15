/*
 *  Example I:  SDI-12 PC Interface
 *
 *  Arduino-based USB dongle translates serial comm from PC to SDI-12 (electrical and timing)
 *  1. Allows user to communicate to SDI-12 devices from a serial terminal emulator (e.g. PuTTY).
 *  2. Able to spy on an SDI-12 bus for troubleshooting comm between datalogger and sensors.
 *  3. Can also be used as a hardware middleman for interfacing software to an SDI-12 sensor.
 *     For example, implementing an SDI-12 datalogger in Python on a PC.  Use verbatim mode with
 *     feedback off in this case.
 *
 *  Note: "translation" means timing and electrical interface.  It does not ensure SDI-12
 *        compliance of commands sent via it.
 *
 * Sketch requires the SDI-12 library from SWRC, modified to add public void forceListen() and
 * public void sendResponse().
 * https://github.com/dwasielewski/Arduino-SDI-12
 *
 * D. Wasielewski, 2016
 * Builds upon work started by:
 * https://github.com/jrzondagh/AgriApps-SDI-12-Arduino-Sensor
 * https://github.com/Jorge-Mendes/Agro-Shield/tree/master/SDI-12ArduinoSensor
 *
 * Known issues:
 *  - Backspace adds a "backspace character" into the serialMsgStr (which gets sent
 *    out on the SDI-12 interface) instead of removing the previous char from it
 *  - Suceptible to noise on the SDI-12 data line; consider hardware filtering or
 *    software error-checking
 *
 */


#define HELPTEXT "OPTIONS:\n\rhelp   : Print this message\n\rmode s : SDI-12 command mode (uppercase and ! automatically corrected) [default]\n\rmode v : verbatim mode (text will be sent verbatim)\n\rfb on  : Enable feedback (characters visible while typing) [default]\n\rfb off : Disable feedback (characters not visible while typing; may be desired for developers)\n\r(else) : send command to SDI-12 bus"

// Requires modified SDI-12 libary with addition of public forceListen() and public sendResponse()
#include "SDI12.h"

// Init an SDI-12 data interface on pin 2 (change for other pin, obviously)
SDI12 sdi(2);

void setup() {
  // Initiate serial connection to PC
  Serial.begin(115200);

  // Initiate serial connection to SDI-12 bus
  sdi.begin();
  delay(500);
  sdi.forceListen();

  // Print help text (may wish to comment out if used for communicating to software)
  Serial.println(HELPTEXT);
}

void loop() {

  static String serialMsgStr;
  static boolean serialMsgReady = false;

  static String sdiMsgStr;
  static boolean sdiMsgReady = false;

  static boolean verbatim = false;
  static boolean feedback = true;


  // -- READ SERIAL (PC COMMS) DATA --
  // If serial data is available, read in a single byte and add it to
  // a String on each iteration
  if (Serial.available()) {
    char inByte1 = Serial.read();
    if (feedback) { Serial.print(inByte1); }
    if (inByte1 == '\r' || inByte1 == '\n') {
      serialMsgReady = true;
    }
    else {
      serialMsgStr += inByte1;
    }
  }

  // -- READ SDI-12 DATA --
  // If SDI-12 data is available, keep reading until full message consumed
  // (Normally I would prefer to allow the loop() to keep executing while the string
  //  is being read in--as the serial example above--but SDI-12 depends on very precise
  //  timing, so it is probably best to let it hold up loop() until the string is complete)
  int avail = sdi.available();
  if (avail < 0) { sdi.clearBuffer(); } // Buffer is full; clear
  else if (avail > 0) {
    for (int a=0; a<avail; a++) {
      char inByte2 = sdi.read();
      if (inByte2 == '\n') {
        sdiMsgReady = true;
      }
      else if (inByte2 == '!') {
        sdiMsgStr += "!";
        sdiMsgReady = true;
      }
      else {
        sdiMsgStr += String(inByte2);
      }
    }
  }



  // Report completed SDI-12 messages back to serial interface
  if (sdiMsgReady) {
    Serial.println(sdiMsgStr);
    // Reset String for next SDI-12 message
    sdiMsgReady = false;
    sdiMsgStr = "";
  }

  // Send completed Serial message as SDI-12 command
  if (serialMsgReady) {
    Serial.println();
    // Check if the serial message is a known command to the SDI-12 interface program
    String lowerMsgStr = serialMsgStr;
    lowerMsgStr.toLowerCase();
    if (lowerMsgStr == "mode v") {
      verbatim = true;
      Serial.println("Verbatim mode; exact text will be sent.  Enter \"mode s\" for SDI-12 command mode.");
    }
    else if (lowerMsgStr == "mode s") {
      verbatim = false;
      Serial.println("SDI-12 command mode; uppercase and ! suffix optional.  Enter \"mode v\" for verbatim mode.");
    }
    else if (lowerMsgStr == "help") {
      Serial.println(HELPTEXT);
    }
    else if (lowerMsgStr == "fb off") {
      feedback = false;
      Serial.println("Feedback off; typed commands will not be visible.  Enter \"fb on\" to enable feedback.");
    }
    else if (lowerMsgStr == "fb on") {
      feedback = true;
      Serial.println("Feedback on; typed commands will be visible.  Enter \"fb off\" to disable feedback.");
    }
    // If not a known command to the SDI-12 interface program, send out on SDI-12 data pin
    else {
      if (verbatim) { sdi.sendCommand(serialMsgStr); }
      else {
        serialMsgStr.toUpperCase();
        sdi.sendCommand(serialMsgStr + "!"); }
    }
    // Reset String for next serial message
    serialMsgReady = false;
    serialMsgStr = "";
  }
}
