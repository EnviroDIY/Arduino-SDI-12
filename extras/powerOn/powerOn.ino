#include <Arduino.h>

#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif

int8_t powerPin = SDI12_POWER_PIN;

void setup() {
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, HIGH);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
}

void loop() {}
