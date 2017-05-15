/*
Example F: Basic data request.
Edited by Ruben Kertesz for ISCO Nile 502 2/10/2016
This is a simple demonstration of the SDI-12 library for Arduino.
This is a very basic (stripped down) example where the user initiates a measurement
and receives the results to a terminal window without typing numerous commands into
the terminal.
The SDI-12 specification is available at: http://www.sdi-12.org/
The library is available at: https://github.com/EnviroDIY/Arduino-SDI-12
The forked library with additional example files is available at: https://github.com/rkertesz/Arduino-SDI-12
The circuit: You should not have more than one SDI-12 device attached for this example.
See:
https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_no_usb.png
or
https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb.png
Written by Kevin M. Smith in 2013.
Contact: SDI12@ethosengineering.org
Extended by Ruben Kertesz in 2016
Contact: github@emnet.net or @rinnamon on twitter
*/


#include "SDI12.h"

#define DATAPIN 12         // change to the proper pin
#define SENSOR_ADDRESS 1

SDI12 mySDI12(DATAPIN);

String sdiResponse = "";
String myCommand = "";

void setup() {
  Serial.begin(9600);
  mySDI12.begin();
}

void loop() {
  do {                           // wait for a response from the serial terminal to do anything
    delay (30);
  }
  while (!Serial.available());
  char nogo = Serial.read();     // simply hit enter in the terminal window or press send and
                                 // the characters get discarded but now the rest of the loop continues

//first command to take a measurement
  myCommand = String(SENSOR_ADDRESS) + "M!";
  Serial.println(myCommand);     // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(30);                     // wait a while for a response

  while (mySDI12.available()) {  // build response string
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(5);
    }
  }
  if (sdiResponse.length() > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();


  delay(1000);                 // delay between taking reading and requesting data
  sdiResponse = "";           // clear the response string


// next command to request data from last measurement
  myCommand = String(SENSOR_ADDRESS) + "D0!";
  Serial.println(myCommand);  // echo command to terminal

  mySDI12.sendCommand(myCommand);
  delay(30);                     // wait a while for a response

  while (mySDI12.available()) {  // build string from response
    char c = mySDI12.read();
    if ((c != '\n') && (c != '\r')) {
      sdiResponse += c;
      delay(5);
    }
  }
  if (sdiResponse.length() > 1) Serial.println(sdiResponse); //write the response to the screen
  mySDI12.clearBuffer();

//now go back to top and wait until user hits enter on terminal window
}
