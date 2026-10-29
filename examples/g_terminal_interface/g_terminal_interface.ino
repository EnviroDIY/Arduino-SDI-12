/**
 * @example{lineno} g_terminal_interface.ino
 * @copyright Stroud Water Research Center
 * @license This example is published under the BSD-3 license.
 * @author Kevin M. Smith <SDI12@ethosengineering.org>
 * @author Ruben Kertesz <github@emnet.net>
 * @author D. Wasielewski
 *
 * @brief Example G: Using an Arduino as an SDI-12 terminal and PC interface
 *
 * This example accepts SDI-12 commands from a serial terminal, forwards SDI-12
 * traffic to the serial port, and can passively monitor communication between
 * another recorder and its sensors. It replaces the former terminal-window and
 * SDI-12-interface examples.
 */

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
#include <EnableInterrupt.h>
#endif

#include <SDI12.h>

#ifndef SDI12_DATA_PIN
#define SDI12_DATA_PIN 7
#endif
#ifndef SDI12_POWER_PIN
#define SDI12_POWER_PIN 22
#endif
#ifndef SDI12_TERMINAL_BAUD
#define SDI12_TERMINAL_BAUD 115200
#endif

uint32_t serialBaud = SDI12_TERMINAL_BAUD; /*!< The baud rate for the serial port */
int8_t   dataPin    = SDI12_DATA_PIN;      /*!< The pin of the SDI-12 data bus */
int8_t   powerPin   = SDI12_POWER_PIN;     /*!< The sensor power pin (or -1) */

/** Define the SDI-12 bus. */
SDI12 mySDI12(dataPin);

bool commandMode = true; /*!< Add a missing ! to commands when true. */
bool feedback    = true; /*!< Echo serial input and print interface messages. */

/** Print the commands understood by this interface. */
void printHelp() {
  Serial.println(F("OPTIONS:"));
  Serial.println(F("help   : Print this message"));
  Serial.println(F("mode s : SDI-12 command mode; a missing ! is added [default]"));
  Serial.println(F("mode v : Verbatim mode; text is sent unchanged"));
  Serial.println(
    F("fb on  : Echo typed characters and print interface messages [default]"));
  Serial.println(F("fb off : Disable input echo and interface messages"));
  Serial.println(F("Anything else is sent to the SDI-12 bus."));
}

/** Process one complete line received from the serial port. */
void processSerialMessage(String& message) {
  String lowerMessage = message;
  lowerMessage.toLowerCase();

  if (lowerMessage == "help") {
    printHelp();
  } else if (lowerMessage == "mode s") {
    commandMode = true;
    if (feedback) {
      Serial.println(F("SDI-12 command mode; a missing ! will be added."));
    }
  } else if (lowerMessage == "mode v") {
    commandMode = false;
    if (feedback) { Serial.println(F("Verbatim mode; text will be sent unchanged.")); }
  } else if (lowerMessage == "fb off") {
    if (feedback) { Serial.println(F("Feedback off.")); }
    feedback = false;
  } else if (lowerMessage == "fb on") {
    feedback = true;
    Serial.println(F("Feedback on."));
  } else {
    if (commandMode && !message.endsWith("!")) { message += '!'; }
    mySDI12.sendCommand(message);
  }

  message = "";
}

/** Read one serial character and assemble complete CR-, LF-, or CRLF-ended lines. */
void readSerialInput() {
  static String serialMessage;
  static bool   previousWasCarriageReturn = false;

  if (!Serial.available()) { return; }

  char incoming = static_cast<char>(Serial.read());
  if (incoming == '\r') {
    previousWasCarriageReturn = true;
    if (feedback) { Serial.println(); }
    if (serialMessage.length() > 0) { processSerialMessage(serialMessage); }
  } else if (incoming == '\n') {
    if (previousWasCarriageReturn) {
      previousWasCarriageReturn = false;
      return;
    }
    if (feedback) { Serial.println(); }
    if (serialMessage.length() > 0) { processSerialMessage(serialMessage); }
  } else if (incoming == '\b' || incoming == 0x7F) {
    previousWasCarriageReturn = false;
    if (serialMessage.length() > 0) {
      serialMessage.remove(serialMessage.length() - 1);
      if (feedback) { Serial.print(F("\b \b")); }
    }
  } else {
    previousWasCarriageReturn = false;
    serialMessage += incoming;
    if (feedback) { Serial.print(incoming); }
  }
}

/** Forward all received SDI-12 traffic to the serial port exactly once. */
void forwardSDI12Input() {
  if (mySDI12.available() < 0) {
    mySDI12.clearBuffer();
    if (feedback) { Serial.println(F("[SDI-12 receive buffer overflow]")); }
    return;
  }

  while (mySDI12.available() > 0) {
    int incoming = mySDI12.read();
    if (incoming >= 0) { Serial.write(static_cast<uint8_t>(incoming)); }
  }
}

void setup() {
  Serial.begin(serialBaud);
  while (!Serial && millis() < 10000L) {}

  if (powerPin >= 0) {
    Serial.println(F("Powering up sensors..."));
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(200);
  }

  Serial.print(F("Opening SDI-12 bus on pin "));
  Serial.print(dataPin);
  Serial.println(F("..."));
  mySDI12.begin();
  delay(500);

#if defined(SDI12_EXTERNAL_PCINT) && (defined(__AVR__) || defined(ARDUINO_ARCH_AVR))
  // Enable interrupts for the receive pin
  pinMode(dataPin, INPUT_PULLUP);
  enableInterrupt(dataPin, SDI12::handleInterrupt, CHANGE);
#endif

  mySDI12.forceListen();

  printHelp();
}

void loop() {
  readSerialInput();
  forwardSDI12Input();
}

// cSpell:ignore Kertesz Wasielewski
