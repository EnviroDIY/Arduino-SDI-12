#include <Arduino.h>

#if defined __AVR__
#include <util/parity.h>  // optimized parity bit handling
#else
// Added MJB: parity function to replace the one specific for AVR from util/parity.h
// http://graphics.stanford.edu/~seander/bithacks.html#ParityNaive
uint8_t parity_even_bit(uint8_t v) {
  uint8_t parity = 0;
  while (v) {
    parity = !parity;
    v      = v & (v - 1);
  }
  return parity;
}
#endif

void printBIN(uint16_t c, uint8_t padding = 8, bool reverse = false) {
  Serial.print("0b");
  for (int8_t b = 0; b < padding; b++) {
    if (reverse) {
      Serial.print(bitRead(c, b));
    } else {
      Serial.print(bitRead(c, padding - 1 - b));
    }
  }
}

void printHEX(uint8_t c) {
  Serial.print("0x");
  if (c < 0x10) { Serial.print("0"); }
  Serial.print(String(c, HEX));
}

// All characters transmitted on the SDI-12 bus must be printable ASCII characters.
// Allowable: " " (space) = 32 = 0x20 -> "~" = 126 = 0x7E
// Exceptions:
//  - sensor responses end with a carriage return (0D hex, 13 decimal) and a line feed
//  (0A hex, 10 decimal) character
//  - 2nd and 3rd character of CRC
//  - the contents of data packets returned by the High Volume Binary command
// Total number of allowable characters = 97, plus CRC/BIN unprintables
uint8_t sdi_chars[97];

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // add allowable characters to the array
  sdi_chars[0] = '\r';
  sdi_chars[1] = '\n';
  uint8_t j    = 2;
  for (uint8_t i = 32; i <= 126; i++) {
    sdi_chars[j] = i;
    j++;
  }
  Serial.println(
    "Character, ASCII (decimal), ASCII (hex), ASCII (binary), Parity, "
    "Character with parity, Transmission Order (LSB first), Bit - Start, Bit 0, "
    "Bit 1, Bit 2, Bit 3, Bit 4, Bit 5, Bit 6, Bit 7 Parity, Bit + Stop");

  // print info about the allowable characters
  for (uint8_t i = 0; i < 97; i++) {
    uint8_t c = sdi_chars[i];
    Serial.print("\"");
    if (c == '\r') {
      Serial.print("\\r");
    } else if (c == '\n') {
      Serial.print("\\n");
    } else if (c == ' ') {
      Serial.print("space");
    } else if (c == '"') {
      Serial.print("quote");
    } else {
      Serial.write(c);
    }
    Serial.print("\", ");
    Serial.print(String(c, DEC));
    Serial.print(", ");
    printHEX(c);
    Serial.print(", ");
    printBIN(c);

    uint8_t parityBit = parity_even_bit(c);  // Calculate the parity bit
    Serial.print(", ");
    Serial.print(parityBit);
    uint8_t c_par = c | (parityBit << 7);  // Add parity bit to the outgoing character
    Serial.print(", ");
    printBIN(c_par);

    // print in reverse to show transmission order (transmitted LSB first)
    Serial.print(", ");
    printBIN(c_par, 8, true);
    // print the levels of each bit with the start and stop bits and separated columns
    // SDI-12 uses inverse logic, so a 0 bit is HIGH and a 1 bit is LOW
    // Start bit is HIGH (equivalent to data bit 0)
    // Stop bit is LOW (equivalent to data bit 1)
    Serial.print(", 1");  // HIGH start bit
    for (int8_t b = 0; b < 8; b++) {
      Serial.print(", ");
      Serial.print(!bitRead(c_par, b));  // Inverse logic! Use !
    }
    Serial.print(", 0");  // LOW stop bit
    Serial.println();
  }
}

void loop() {}
