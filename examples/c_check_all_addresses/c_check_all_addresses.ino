/*
Example C: Checks all addresses for active sensors, and prints their status to the serial port. 
 
 This is a simple demonstration of the SDI-12 library for Arduino.
 
 It discovers the address of all sensors active on a single bus. 
 
 Each sensor should have a unique address already - if not, multiple sensors may respond simultaenously
 to the same request and the output will not be readable by the Arduino. 
 
 To address a sensor, please see Example B: b_address_change.ino
 
 The SDI-12 specification is available at: http://www.sdi-12.org/
 The library is available at: https://github.com/StroudCenter/arduino-SDI-12
 
 The circuit: You  may use one or more pre-adressed sensors.  
 See:
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_usb_multiple_sensors.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb_multiple_sensors.png
 
 Written by Kevin M. Smith in 2013. 
 Contact: SDI12@ethosengineering.org
 */


#include <SDI12.h>

#define DATAPIN 9         // change to the proper pin
SDI12 mySDI12(DATAPIN); 

// keeps track of active addresses
// each bit represents an address:
// 1 is active (taken), 0 is inactive (available)
// setTaken('A') will set the proper bit for sensor 'A'
// set 
byte addressRegister[8] = { 
  0B00000000, 
  0B00000000, 
  0B00000000, 
  0B00000000, 
  0B00000000, 
  0B00000000, 
  0B00000000, 
  0B00000000 
}; 


void setup(){
  Serial.begin(9600); 
  mySDI12.begin(); 
  delay(500); // allow things to settle
  
  /*
      Quickly Scan the Address Space
  */
  
  for(byte i = '0'; i <= '9'; i++) if(checkActive(i)) setTaken(i);   // scan address space 0-9

  for(byte i = 'a'; i <= 'z'; i++) if(checkActive(i)) setTaken(i);   // scan address space a-z

  for(byte i = 'A'; i <= 'Z'; i++) if(checkActive(i)) setTaken(i);   // scan address space A-Z

  /*
      For each active sensor, have it print it's current information. 
  */
  
  // scan address space 0-9
  for(char i = '0'; i <= '9'; i++) if(isTaken(i)){
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(":");
    printInfo(i);   
    Serial.println(); 
  }
  
  // scan address space a-z
  for(char i = 'a'; i <= 'z'; i++) if(isTaken(i)){
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(": ");
    printInfo(i);   
    Serial.println(); 
  } 
  
  // scan address space A-Z
  for(char i = 'A'; i <= 'Z'; i++) if(isTaken(i)){
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(":");
    printInfo(i);   
    Serial.println();  
  };  
}

 
// this checks for activity at a particular address     
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(char i){              
  Serial.print("Checking address ");
  Serial.print(i);
  Serial.print("..."); 
  String myCommand = "";
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    if(mySDI12.available()>1) break;
    delay(30); 
  }
  if(mySDI12.available()>2){       // if it hears anything it assumes the address is occupied
    Serial.println("Occupied");
    mySDI12.flush(); 
    return true;
  } 
  else {
    Serial.println("Vacant");           // otherwise it is vacant. 
    mySDI12.flush(); 
  }
  return false; 
}


// this sets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken. 
boolean setTaken(byte i){          
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61. 
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] |= (1 << k); 
  return !initStatus; // return false if already taken
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL. 
// this unsets the bit in the proper location within the addressRegister
// to record that the sensor is active and the address is taken. 
boolean setVacant(byte i){
  boolean initStatus = isTaken(i);
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61. 
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  addressRegister[j] &= ~(1 << k); 
  return initStatus; // return false if already vacant
}


// this quickly checks if the address has already been taken by an active sensor           
boolean isTaken(byte i){         
  i = charToDec(i); // e.g. convert '0' to 0, 'a' to 10, 'Z' to 61. 
  byte j = i / 8;   // byte #
  byte k = i % 8;   // bit #
  return addressRegister[j] & (1<<k); // return bit status
}

// gets identification information from a sensor, and prints it to the serial port
// expects a character between '0'-'9', 'a'-'z', or 'A'-'Z'. 
char printInfo(char i){
  int j; 
  String command = "";
  command += (char) i; 
  command += "I!";
  for(j = 0; j < 1; j++){
    mySDI12.sendCommand(command);
    delay(30); 
    if(mySDI12.available()>1) break;
    if(mySDI12.available()) mySDI12.read(); 
  }
  
  mySDI12.read(); //consume sensor address (you can keep it if you'd like)

  while(mySDI12.available()){
    Serial.write(mySDI12.read()); 
    delay(5); 
  } 
}

// converts allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
// to a decimal number between 0 and 61 (inclusive) to cover the 62 possible addresses
byte charToDec(char i){
  if((i >= '0') && (i <= '9')) return i - '0';
  if((i >= 'a') && (i <= 'z')) return i - 'a' + 10;
  if((i >= 'A') && (i <= 'Z')) return i - 'A' + 37;
}

// THIS METHOD IS UNUSED IN THIS EXAMPLE, BUT IT MAY BE HELPFUL. 
// maps a decimal number between 0 and 61 (inclusive) to 
// allowable address characters '0'-'9', 'a'-'z', 'A'-'Z',
char decToChar(byte i){
  if((i >= 0) && (i <= 9)) return i + '0';
  if((i >= 10) && (i <= 36)) return i + 'a' - 10;
  if((i >= 37) && (i <= 62)) return i + 'A' - 37;
}

void loop(){} // this sketch does not loop

