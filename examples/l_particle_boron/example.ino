#include "SDI12_boards.h"

// This #include statement was automatically added by the Particle IDE.
#include "SDI12.h"


void printInfo(SDI12 sdi, char i) {
  String command = "";
  command += (char)i;
  command += "I!";
  sdi.sendCommand(command);
  sdi.clearBuffer();
  delay(30);

  if(sdi.available()){
    String msg=String(i);
    msg+=": ";
  while (sdi.available()) {
    msg+=(char)sdi.read();
    delay(10);  // 1 character ~ 7.5ms
  }
  Particle.publish("address", String(i));
  Particle.publish("msg", msg);
  }
}

// this checks for activity at a particular address
// expects a char, '0'-'9', 'a'-'z', or 'A'-'Z'
boolean checkActive(SDI12 sdi, char i) {
  String myCommand = "";
  myCommand        = "";
  myCommand += (char)i;  // sends basic 'acknowledge' command [address][!]
  myCommand += "!";

  for (int j = 0; j < 3; j++) {  // goes through three rapid contact attempts
    sdi.sendCommand(myCommand);
    sdi.clearBuffer();
    delay(30);
    if (sdi.available()) {  // If we here anything, assume we have an active sensor
      return true;
    }
  }
  sdi.clearBuffer();
  return false;
}

void scanAddressSpace(SDI12 sdi) {
  // scan address space 0-9
  for (char i = '0'; i <= '9'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); }
  // scan address space a-z
  for (char i = 'a'; i <= 'z'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); }
  // scan address space A-Z
  for (char i = 'A'; i <= 'Z'; i++)
    if (checkActive(sdi, i)) { printInfo(sdi, i); };
}

void setup(){
    Particle.publish("ticks", String(System.ticksPerMicrosecond()));    

}
void loop(){
    SDI12 mySDI12(D2);
    mySDI12.begin();
    scanAddressSpace(mySDI12);
    mySDI12.end();
    delay(100000);
}