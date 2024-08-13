#include <SDI12.h>

int8_t dataPin = 7; /*!< The pin of the SDI-12 data bus */

// Create object by which to communicate with the SDI-12 bus on SDIPIN
SDI12 slaveSDI12(dataPin);

void setup() {
  Serial.begin(115200);
  slaveSDI12.begin();
  delay(500);
  slaveSDI12.forceListen();  // sets SDIPIN as input to prepare for incoming message
  Serial.println("Starting SDI-12 Spy");
}

void loop() {
  while (slaveSDI12.available()) {
    int readChar = slaveSDI12.read();
    Serial.write(readChar);
    // if (readChar == '\n') {
    //   slaveSDI12.forceListen();
    // } else {
    //   delay(10);// 1 character ~ 7.5ms
    // }
  }
}
