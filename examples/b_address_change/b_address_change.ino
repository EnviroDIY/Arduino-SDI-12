/*
Example B: Changing the address of a sensor. 
 
 This is a simple demonstration of the SDI-12 library for arduino.
 It discovers the address of the attached sensor and allows you to change it.
 
 The SDI-12 specification is available at: http://www.sdi-12.org/
 The library is available at: https://github.com/StroudCenter/arduino-SDI-12
 
 The circuit: You should not have more than one SDI-12 device attached for this example. 
 See: 
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/basic_setup_no_usb.png
 or
 https://raw.github.com/Kevin-M-Smith/SDI-12-Circuit-Diagrams/master/compat_setup_usb.png
 
 Written by Kevin M. Smith in 2013. 
 Contact: SDI12@ethosengineering.org
 */


#include <SDI12.h>

#define DATAPIN 9         // change to the proper pin
SDI12 mySDI12(DATAPIN); 

String myCommand = "";   // empty to start
char oldAddress = '!';   // invalid address as placeholder

void setup(){
  Serial.begin(9600); 
  mySDI12.begin(); 
}

void loop(){
  boolean found = false;   // have we identified the sensor yet?

  for(byte i = '0'; i <= '9'; i++){    // scan address space 0-9
    if(found) break;
    if(checkActive(i)){
      found = true;  
      oldAddress = i; 
    }
  }

  for(byte i = 'a'; i <= 'z'; i++){    // scan address space a-z
    if(found) break;
    if(checkActive(i)){
      found = true;  
      oldAddress = i; 
    }
  }

  for(byte i = 'A'; i <= 'Z'; i++){    // scan address space A-Z
    if(found) break;
    if(checkActive(i)){
      found = true;  
      oldAddress = i; 
    }
  }

  if(!found){ 
    Serial.println("No sensor detected. Check physical connections."); // couldn't find a sensor. check connections..
  }
  else{
    Serial.print("Sensor active at address ");                        // found a sensor!
    Serial.print(oldAddress); 
    Serial.println("."); 

    Serial.println("Enter new address.");                             // prompt for a new address
    while(!Serial.available()); 
    char newAdd= Serial.read();
    
    // wait for valid response
    while( ((newAdd<'0') || (newAdd>'9')) && ((newAdd<'a') || (newAdd>'z')) && ((newAdd<'A') || (newAdd>'Z'))){
      if(!(newAdd =='\n') || (newAdd =='\r') || (newAdd ==' ')) { 
        Serial.println("Not a valid address. Please enter '0'-'9', 'a'-'A', or 'z'-'Z'."); 
      }
      while(!Serial.available()); 
      newAdd = Serial.read();
    } 

/* the syntax of the change address command is:
[currentAddress]A[newAddress]! */
   
    Serial.println("Readdressing sensor.");
    myCommand = "";
    myCommand += (char) oldAddress;
    myCommand += "A";
    myCommand += (char) newAdd;
    myCommand += "!";
    mySDI12.sendCommand(myCommand);

/* wait for the response then throw it away by
clearing the buffer with flush()  */
    delay(300);
    mySDI12.flush(); 
    
    Serial.println("Success. Rescanning for verification."); 
  }
}


boolean checkActive(byte i){              // this checks for activity at a particular address
  Serial.print("Checking address ");
  Serial.print((char)i);
  Serial.print("..."); 
  myCommand = "";
  myCommand += (char) i;                 // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for(int j = 0; j < 3; j++){            // goes through three rapid contact attempts
    mySDI12.sendCommand(myCommand);
    if(mySDI12.available()>1) break;
    delay(30); 
  }
  if(mySDI12.available()>2){             // if it hears anything it assumes the address is occupied
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

