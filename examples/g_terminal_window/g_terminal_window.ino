/*
Example G: Using the Arduino as a command terminal for SDI-12 sensors.

Edited by Ruben Kertesz for ISCO Nile 502 2/10/2016…//functions as terminal

This is a simple demonstration of the SDI-12 library for Arduino.
It's purpose is to allow a user to interact with an SDI-12 sensor directly,
issuing commands through a serial terminal window.
The SDI-12 specification is available at: http://www.sdi-12.org/
The master library is available at: https://github.com/EnviroDIY/Arduino-SDI-12
The forked library with additional example files is available at: https://github.com/rkertesz/Arduino-SDI-12
The circuit: It is recommended that you not have more than one SDI-12 device attached for this example.
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
SDI12 mySDI12(DATAPIN);

char inByte = 0;
String sdiResponse = "";
String myCommand = "";

void setup(){
  Serial.begin(9600);
  mySDI12.begin();
}

void loop(){
    if (Serial.available()) {
        inByte = Serial.read();
        if((inByte!='\n') && (inByte!='\r')) { //read all values entered in terminal window before enter
          myCommand += inByte;
          delay(5);
        }
    }

  if(inByte == '\r'){ // once we press enter, send string to SDI sensor/probe
    inByte = 0;
    Serial.println(myCommand);
    mySDI12.sendCommand(myCommand);
    delay(30);                     // wait a while for a response

    while(mySDI12.available()){    // build a string of the response
      char c = mySDI12.read();
      if((c!='\n') && (c!='\r')) {
        sdiResponse += c;
        delay(5);
      }
    }
    if (sdiResponse.length()>1) Serial.println(sdiResponse); //write the response to the screen

    mySDI12.clearBuffer(); //clear the line
    myCommand = "";
    sdiResponse = "";
  }
}
