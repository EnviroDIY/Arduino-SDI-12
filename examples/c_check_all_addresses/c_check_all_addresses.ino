/*
########################
#        OVERVIEW      #
########################

 Example C: Checks all addresses for active sensors, and prints their status to the serial port.

 This is a simple demonstration of the SDI-12 library for Arduino.

 It discovers the address of all sensors active and attached to the board.
 THIS CAN BE *REALLY* SLOW TO RUN!!!

 Each sensor should have a unique address already - if not, multiple sensors may respond simultaenously
 to the same request and the output will not be readable by the Arduino.

 To address a sensor, please see Example B: b_address_change.ino

#########################
#      THE CIRCUIT      #
#########################

 You  may use one or more pre-adressed sensors.

 See:
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_usb_multiple_sensors.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb_multiple_sensors.png

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


#include "SDI12.h"

#define POWERPIN -1     // change to the proper pin (or -1 if not switching power)
#define FirstPin 5      // change to lowest pin number on your board
#define LastPin 24       // change to highest pin number on your board

// gets identification information from a sensor, and prints it to the serial port
// expects a character between '0'-'9', 'a'-'z', or 'A'-'Z'.
void printInfo(SDI12 sdi, char i){
  int j;
  String command = "";
  command += (char) i;
  command += "I!";
  for(j = 0; j < 1; j++){
    sdi.sendCommand(command);
    sdi.clearBuffer();
    delay(30);
    if(sdi.available()>1) break;
    if(sdi.available()) sdi.read();
  }

  Serial.print("  --");
  Serial.print(i);
  Serial.print("--  ");

  while(sdi.available()){
    Serial.write(sdi.read());
    delay(5);
  }
}


// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(SDI12 sdi, char i){
  String myCommand = "";
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    sdi.sendCommand(myCommand);
    sdi.clearBuffer();
    delay(30);
    if(sdi.available()>1) break;
  }
  if(sdi.available()>2){       // if it hears anything it assumes the address is occupied
    return true;
  }
  return false;
}

void scanAddressSpace(SDI12 sdi){
  // scan address space 0-9
  for(char i = '0'; i <= '9'; i++) if(checkActive(sdi, i)){
    printInfo(sdi, i);
  }
  // scan address space a-z
  for(char i = 'a'; i <= 'z'; i++) if(checkActive(sdi, i)){
    printInfo(sdi, i);
  }
  // scan address space A-Z
  for(char i = 'A'; i <= 'Z'; i++) if(checkActive(sdi, i)){
    printInfo(sdi, i);
  };
}

void setup(){
  Serial.begin(57600);
  Serial.println("//\n// Start Search for SDI-12 Devices \n// -----------------------");

  // Power the sensors;
  #if POWERPIN > 0
    pinMode(POWERPIN, OUTPUT);
    digitalWrite(POWERPIN, HIGH);
    delay(200);
  #endif

  for (uint8_t pin = FirstPin; pin < LastPin; pin++)
  {
    if (pin != POWERPIN){
      pinMode(pin, INPUT);
      SDI12 mySDI12(pin);
      mySDI12.begin();
      Serial.print("Checking pin ");
      Serial.print(pin);
      Serial.println("...");
      scanAddressSpace(mySDI12);
      mySDI12.end();
    }
  }

  Serial.println("\n//\n// End Search for SDI-12 Devices \n// ---------------------");

  // Cut power
  digitalWrite(POWERPIN, LOW);

}

void loop(){}
