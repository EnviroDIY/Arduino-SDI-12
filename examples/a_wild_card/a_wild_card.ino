/*
########################
#        OVERVIEW      #
########################

 Example A: Using the wildcard.

 This is a simple demonstration of the SDI-12 library for Arduino.
 It requests information about the attached sensor, including its address and manufacturer info.

#########################
#      THE CIRCUIT      #
#########################

 You should not have more than one SDI-12 device attached for this example.

 See:
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_no_usb.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb.png

###########################
#      COMPATIBILITY      #
###########################

 This library requires the use of pin change interrupts (PCINT).
 Not all Arduino boards have the same pin capabilities.
 The known compatibile pins for common variants are shown below.

 Arduino Uno: 	All pins.

 Arduino Mega or Mega 2560:
 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62),
 A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).

 Arduino Leonardo:
 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

#########################
#      RESOURCES        #
#########################

 Written by Kevin M. Smith in 2013.
 Contact: SDI12@ethosengineering.org

 The SDI-12 specification is available at: http://www.sdi-12.org/
 The library is available at: https://github.com/EnviroDIY/Arduino-SDI-12
*/

#include "SDI12.h"

#define DATAPIN 9         // change to the proper pin
SDI12 mySDI12(DATAPIN);

/*
  '?' is a wildcard character which asks any and all sensors to respond
  'I' indicates that the command wants information about the sensor
  '!' finishes the command
*/
String myCommand = "?I!";

void setup(){
  Serial.begin(9600);
  mySDI12.begin();
}

void loop(){
  mySDI12.sendCommand(myCommand);
  delay(300);                     // wait a while for a response
  while(mySDI12.available()){    // write the response to the screen
    Serial.write(mySDI12.read());
  }
  delay(3000); // print again in three seconds
}
