/**
 * @file SDI12.cpp
 * @copyright (c) 2013-2020 Stroud Water Research Center (SWRC)
 *                          and the EnviroDIY Development Team
 * @date August 2013
 * @author Kevin M.Smith <SDI12@ethosengineering.org>
 *
 * @brief This file implements the main class for the SDI-12 implementation.
 *
 * ========================== Arduino SDI-12 ==================================
 *
 * An Arduino library for SDI-12 communication with a wide variety of environmental
 * sensors. This library provides a general software solution, without requiring any
 * additional hardware.
 *
 * ======================== Attribution & License =============================
 *
 * Copyright (C) 2013  Stroud Water Research Center
 * Available at https://github.com/EnviroDIY/Arduino-SDI-12
 *
 * Authored initially in August 2013 by:
 *          Kevin M. Smith (http://ethosengineering.org)
 *          Inquiries: SDI12@ethosengineering.org
 *
 * Modified 2017 by Manuel Jimenez Buendia to work with ARM based processors (Arduino
 * Zero)
 *
 * Maintenance and merging 2017 by Sara Damiano
 *
 * based on the SoftwareSerial library (formerly NewSoftSerial), authored by:
 *         ladyada (http://ladyada.net)
 *         Mikal Hart (http://www.arduiniana.org)
 *         Paul Stoffregen (http://www.pjrc.com)
 *         Garrett Mace (http://www.macetech.com)
 *         Brett Hagman (http://www.roguerobotics.com/)
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this library; if not, write to the Free Software Foundation, Inc., 51 Franklin
 * Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include "SDI12.h"  //  Header file for this library

/* ================  Set static constants ===========================================*/

// Pointer to active SDI12 object
SDI12* SDI12::_activeObject = NULL;
// Timer functions
SDI12Timer SDI12::sdi12timer;

// The size of a bit in microseconds
// 1200 baud = 1200 bits/second ~ 833.333 µs/bit
const uint16_t SDI12::bitWidth_micros = (uint16_t)833;
// The required "break" before sending commands, >= 12ms
const uint16_t SDI12::lineBreak_micros = (uint16_t)12300;
// The required mark before a command or response, >= 8.33ms
const uint16_t SDI12::marking_micros = (uint16_t)8500;

// the width of a single bit in "ticks" of the cpu clock.
const uint8_t SDI12::txBitWidth = TICKS_PER_BIT;
// A fudge factor to make things work
const uint8_t SDI12::rxWindowWidth = RX_WINDOW_FUDGE;
// The number of bits per tick, shifted by 2^10.
const uint8_t SDI12::bitsPerTick_Q10 = BITS_PER_TICK_Q10;
// A mask waiting for a start bit; 0b11111111
const uint8_t SDI12::WAITING_FOR_START_BIT = 0xFF;

uint16_t SDI12::prevBitTCNT;  // previous RX transition in micros
uint8_t  SDI12::rxState;      // 0: got start bit; >0: bits rcvd
uint8_t  SDI12::rxMask;       // bit mask for building received character
uint8_t  SDI12::rxValue;      // character being built

uint16_t SDI12::mul8x8to16(uint8_t x, uint8_t y) {
  return x * y;
}

uint16_t SDI12::bitTimes(uint8_t dt) {
  return mul8x8to16(dt + rxWindowWidth, bitsPerTick_Q10) >> 10;
}


/* ================ Buffer Setup ====================================================*/
uint8_t          SDI12::_rxBuffer[SDI12_BUFFER_SIZE];  // The Rx buffer
volatile uint8_t SDI12::_rxBufferTail = 0;             // index of buff tail
volatile uint8_t SDI12::_rxBufferHead = 0;             // index of buff head


/* ================ Reading from the SDI-12 Buffer ==================================*/

// reveals the number of characters available in the buffer
int SDI12::available() {
  if (_bufferOverflow) return -1;
  return (_rxBufferTail + SDI12_BUFFER_SIZE - _rxBufferHead) % SDI12_BUFFER_SIZE;
}

// reveals the next character in the buffer without consuming
int SDI12::peek() {
  if (_rxBufferHead == _rxBufferTail) return -1;  // Empty buffer? If yes, -1
  return _rxBuffer[_rxBufferHead];                // Otherwise, read from "head"
}

// a public function that clears the buffer contents and resets the status of the buffer
// overflow.
void SDI12::clearBuffer() {
  _rxBufferHead = _rxBufferTail = 0;
  _bufferOverflow               = false;
}

// reads in the next character from the buffer (and moves the index ahead)
int SDI12::read() {
  _bufferOverflow = false;                        // Reading makes room in the buffer
  if (_rxBufferHead == _rxBufferTail) return -1;  // Empty buffer? If yes, -1
  uint8_t nextChar = _rxBuffer[_rxBufferHead];    // Otherwise, grab char at head
  _rxBufferHead    = (_rxBufferHead + 1) % SDI12_BUFFER_SIZE;  // increment head
  return nextChar;                                             // return the char
}

// these functions hide the stream equivalents to return a custom timeout value
int SDI12::peekNextDigit(LookaheadMode lookahead, bool detectDecimal) {
  int c;
  while (1) {
    c = timedPeek();

    if (c < 0 || c == '-' || (c >= '0' && c <= '9') || (detectDecimal && c == '.'))
      return c;

    switch (lookahead) {
      case SKIP_NONE: return -1;  // Fail code.
      case SKIP_WHITESPACE:
        switch (c) {
          case ' ':
          case '\t':
          case '\r':
          case '\n': break;
          default: return -1;  // Fail code.
        }
      case SKIP_ALL: break;
    }
    read();  // discard non-numeric
  }
}

long SDI12::parseInt(LookaheadMode lookahead, char ignore) {
  bool     isNegative = false;
  uint16_t value      = 0;
  int      c;

  c = peekNextDigit(lookahead, false);
  // ignore non numeric leading characters
  if (c < 0) return TIMEOUT;  // TIMEOUT returned if timeout
  //  THIS IS THE ONLY DIFFERENCE BETWEEN THIS FUNCTION AND THE STREAM DEFAULT!

  do {
    if (c == ignore) {  // ignore this character
    } else if (c == '-') {
      isNegative = true;
    } else if (c >= '0' && c <= '9') {  // is c a digit?
      value = value * 10 + c - '0';
    }
    read();  // consume the character we got with peek
    c = timedPeek();
  } while ((c >= '0' && c <= '9') || c == ignore);

  if (isNegative) value = -value;
  return value;
}

// the same as parseInt but returns a floating point value
float SDI12::parseFloat(LookaheadMode lookahead, char ignore) {
  bool  isNegative = false;
  bool  isFraction = false;
  long  value      = 0;
  int   c;
  float fraction = 1.0;

  c = peekNextDigit(lookahead, true);
  // ignore non numeric leading characters
  if (c < 0) return TIMEOUT;  // TIMEOUT returned if timeout
  //  THIS IS THE ONLY DIFFERENCE BETWEEN THIS FUNCTION AND THE STREAM DEFAULT!

  do {
    if (c == ignore) {  // ignore
    } else if (c == '-') {
      isNegative = true;
    } else if (c == '.') {
      isFraction = true;
    } else if (c >= '0' && c <= '9') {  // is c a digit?
      value = value * 10 + c - '0';
      if (isFraction) fraction *= 0.1;
    }
    read();  // consume the character we got with peek
    c = timedPeek();
  } while ((c >= '0' && c <= '9') || (c == '.' && !isFraction) || c == ignore);

  if (isNegative) value = -value;
  if (isFraction)
    return value * fraction;
  else
    return value;
}

/* ================ Constructor, Destructor, begin(), end(), and timeout ============*/
// Constructor
SDI12::SDI12() {
  _dataPin        = -1;
  _bufferOverflow = false;
  // SDI-12 protocol says sensors must respond within 15 milliseconds
  // We'll bump that up to 150, just for good measure, but we don't want to
  // wait the whole stream default of 1s for a response.
  setTimeout(150);
  // Because SDI-12 is mostly used for environmental sensors, we want to be able
  // to distinguish between the '0' that parseInt and parseFloat usually return
  // on timeouts and a real measured 0 value.  So we force the timeout response
  // to be -9999, which is not a common value for most variables measured by
  // in-site environmental sensors.
  setTimeoutValue(-9999);
}
SDI12::SDI12(int8_t dataPin) {
  _dataPin        = dataPin;
  _bufferOverflow = false;
  // SDI-12 protocol says sensors must respond within 15 milliseconds
  // We'll bump that up to 150, just for good measure, but we don't want to
  // wait the whole stream default of 1s for a response.
  setTimeout(150);
  // Because SDI-12 is mostly used for environmental sensors, we want to be able
  // to distinguish between the '0' that parseInt and parseFloat usually return
  // on timeouts and a real measured 0 value.  So we force the timeout response
  // to be -9999, which is not a common value for most variables measured by
  // in-site environmental sensors.
  setTimeoutValue(-9999);
}

// Destructor
SDI12::~SDI12() {
  setState(SDI12_DISABLED);
  if (isActive()) { _activeObject = NULL; }
  // Set the timer prescalers back to original values
  // NOTE:  This does NOT reset SAMD board pre-scalers!
  sdi12timer.resetSDI12TimerPrescale();
}

// Begin
void SDI12::begin() {
  // setState(SDI12_HOLDING);
  setActive();
  // Set up the prescaler as needed for timers
  // This function is defined in SDI12_boards.h
  sdi12timer.configSDI12TimerPrescale();
}
void SDI12::begin(int8_t dataPin) {
  _dataPin = dataPin;
  begin();
}

// End
void SDI12::end() {
  setState(SDI12_DISABLED);
  _activeObject = NULL;
  // Set the timer prescalers back to original values
  // NOTE:  This does NOT reset SAMD board pre-scalers!
  sdi12timer.resetSDI12TimerPrescale();
}

// Set the timeout return
void SDI12::setTimeoutValue(int16_t value) {
  TIMEOUT = value;
}

// Set the data pin for the SDI-12 instance
void SDI12::setDataPin(int8_t dataPin) {
  _dataPin = dataPin;
}

// Return the data pin for the SDI-12 instance
int8_t SDI12::getDataPin() {
  return _dataPin;
}


/* ================ Using more than one SDI-12 object ===============================*/
// a method for setting the current object as the active object
bool SDI12::setActive() {
  if (_activeObject != this) {
    setState(SDI12_HOLDING);
    _activeObject = this;
    return true;
  }
  return false;
}

// a method for checking if this object is the active object
bool SDI12::isActive() {
  return this == _activeObject;
}


/* ================ Data Line States ================================================*/
// Processor specific parity and interrupts
#if defined __AVR__
#include <avr/interrupt.h>  // interrupt handling
#include <util/parity.h>    // optimized parity bit handling
#else
// Added MJB: parity function to replace the one specific for AVR from util/parity.h
// http://graphics.stanford.edu/~seander/bithacks.html#ParityNaive
uint8_t SDI12::parity_even_bit(uint8_t v) {
  uint8_t parity = 0;
  while (v) {
    parity = !parity;
    v      = v & (v - 1);
  }
  return parity;
}
#endif

// a helper function to switch pin interrupts on or off
void SDI12::setPinInterrupts(bool enable) {
#if defined(ARDUINO_ARCH_SAMD) || defined(ESP32) || defined(ESP8266)
  // Merely need to attach the interrupt function to the pin
  if (enable) attachInterrupt(digitalPinToInterrupt(_dataPin), handleInterrupt, CHANGE);
  // Merely need to detach the interrupt function from the pin
  else
    detachInterrupt(digitalPinToInterrupt(_dataPin));

#elif defined(__AVR__) && not defined(SDI12_EXTERNAL_PCINT)
  if (enable) {
    // Enable interrupts on the register with the pin of interest
    *digitalPinToPCICR(_dataPin) |= (1 << digitalPinToPCICRbit(_dataPin));
    // Enable interrupts on the specific pin of interest
    // The interrupt function is actually attached to the interrupt way down in
    // section 7.5
    *digitalPinToPCMSK(_dataPin) |= (1 << digitalPinToPCMSKbit(_dataPin));
  } else {
    // Disable interrupts on the specific pin of interest
    *digitalPinToPCMSK(_dataPin) &= ~(1 << digitalPinToPCMSKbit(_dataPin));
    if (!*digitalPinToPCMSK(_dataPin)) {
      // If there are no other pins on the register left with enabled interrupts,
      // disable the whole register
      *digitalPinToPCICR(_dataPin) &= ~(1 << digitalPinToPCICRbit(_dataPin));
    }
    // We don't detach the function from the interrupt for AVR processors
  }
#else
  if (enable) {
    return;
  } else {
    return;
  }
#endif
}

// sets the state of the SDI-12 object.
void SDI12::setState(SDI12_STATES state) {
  switch (state) {
    case SDI12_HOLDING: {
      pinMode(_dataPin, INPUT);     // Turn off the pull-up resistor
      pinMode(_dataPin, OUTPUT);    // Pin mode = output
      digitalWrite(_dataPin, LOW);  // Pin state = low - marking
      setPinInterrupts(false);      // Interrupts disabled on data pin
      break;
    }
    case SDI12_TRANSMITTING: {
      pinMode(_dataPin, INPUT);   // Turn off the pull-up resistor
      pinMode(_dataPin, OUTPUT);  // Pin mode = output
      setPinInterrupts(false);    // Interrupts disabled on data pin
      break;
    }
    case SDI12_LISTENING: {
      digitalWrite(_dataPin, LOW);  // Pin state = low (turns off pull-up)
      pinMode(_dataPin, INPUT);     // Pin mode = input, pull-up resistor off
      interrupts();                 // Enable general interrupts
      setPinInterrupts(true);       // Enable Rx interrupts on data pin
      rxState = WAITING_FOR_START_BIT;
      break;
    }
    default:  // SDI12_DISABLED or SDI12_ENABLED
    {
      digitalWrite(_dataPin, LOW);  // Pin state = low (turns off pull-up)
      pinMode(_dataPin, INPUT);     // Pin mode = input, pull-up resistor off
      setPinInterrupts(false);      // Interrupts disabled on data pin
      break;
    }
  }
}

// forces a SDI12_HOLDING state.
void SDI12::forceHold() {
  setState(SDI12_HOLDING);
}

// forces a SDI12_LISTENING state.
void SDI12::forceListen() {
  setState(SDI12_LISTENING);
}


/* ================ Waking Up and Talking To Sensors ================================*/
// this function wakes up the entire sensor bus
void SDI12::wakeSensors(int8_t extraWakeTime) {
  setState(SDI12_TRANSMITTING);
  // Universal interrupts can be on while the break and marking happen because
  // timings for break and from the recorder are not critical.
  // Interrupts on the pin are disabled for the entire transmitting state
  digitalWrite(_dataPin, HIGH);         // break is HIGH
  delayMicroseconds(lineBreak_micros);  // Required break of 12 milliseconds (12,000 µs)
  delay(extraWakeTime);                 // allow the sensors to wake
  digitalWrite(_dataPin, LOW);          // marking is LOW
  delayMicroseconds(marking_micros);  // Required marking of 8.33 milliseconds(8,333 µs)
}

// this function writes a character out on the data line
void SDI12::writeChar(uint8_t outChar) {
  uint8_t currentTxBitNum = 0;  // first bit is start bit
  uint8_t bitValue        = 1;  // start bit is HIGH (inverse parity...)

  noInterrupts();  // _ALL_ interrupts disabled so timing can't be shifted

  sdi12timer_t t0 = READTIME;  // start time

  digitalWrite(
    _dataPin,
    HIGH);  // immediately get going on the start bit
            // this gives us 833µs to calculate parity and position of last high bit
  currentTxBitNum++;

  uint8_t parityBit = parity_even_bit(outChar);  // Calculate the parity bit
  outChar |= (parityBit << 7);  // Add parity bit to the outgoing character

  // Calculate the position of the last bit that is a 0/HIGH (ie, HIGH, not marking)
  // That bit will be the last time-critical bit.  All bits after that can be
  // sent with interrupts enabled.

  uint8_t lastHighBit =
    9;  // The position of the last bit that is a 0 (ie, HIGH, not marking)
  uint8_t msbMask = 0x80;  // A mask with all bits at 1
  while (msbMask & outChar) {
    lastHighBit--;
    msbMask >>= 1;
  }

  // Hold the line for the rest of the start bit duration

  while ((uint8_t)(READTIME - t0) < txBitWidth) {}
  t0 = READTIME;  // advance start time

  // repeat for all data bits until the last bit different from marking
  while (currentTxBitNum++ < lastHighBit) {
    bitValue = outChar & 0x01;  // get next bit in the character to send
    if (bitValue) {
      digitalWrite(_dataPin, LOW);  // set the pin state to LOW for 1's
    } else {
      digitalWrite(_dataPin, HIGH);  // set the pin state to HIGH for 0's
    }
    // Hold the line for this bit duration
    while ((uint8_t)(READTIME - t0) < txBitWidth) {}
    t0 = READTIME;  // start time

    outChar = outChar >> 1;  // shift character to expose the following bit
  }

  // Set the line low for the all remaining 1's and the stop bit
  digitalWrite(_dataPin, LOW);

  interrupts();  // Re-enable universal interrupts as soon as critical timing is past

  // Hold the line low until the end of the 10th bit
  uint8_t bitTimeRemaining = txBitWidth * (10 - lastHighBit);
  while ((uint8_t)(READTIME - t0) < bitTimeRemaining) {}
}

// The typical write functionality for a stream object
// This allows you to use the stream print functions to send commands out on
// the SDI-12, line, but it will not wake the sensors in advance of the command.
size_t SDI12::write(uint8_t byte) {
  setState(SDI12_TRANSMITTING);
  writeChar(byte);            // write the character/byte
  setState(SDI12_LISTENING);  // listen for reply
  return 1;                   // 1 character sent
}

// this function sends out the characters of the String cmd, one by one
void SDI12::sendCommand(String& cmd, int8_t extraWakeTime) {
  wakeSensors(extraWakeTime);  // wake up sensors
  for (int unsigned i = 0; i < cmd.length(); i++) {
    writeChar(cmd[i]);  // write each character
  }
  setState(SDI12_LISTENING);  // listen for reply
}

void SDI12::sendCommand(const char* cmd, int8_t extraWakeTime) {
  wakeSensors(extraWakeTime);  // wake up sensors
  for (int unsigned i = 0; i < strlen(cmd); i++) {
    writeChar(cmd[i]);  // write each character
  }
  setState(SDI12_LISTENING);  // listen for reply
}

void SDI12::sendCommand(FlashString cmd, int8_t extraWakeTime) {
  wakeSensors(extraWakeTime);  // wake up sensors
  for (int unsigned i = 0; i < strlen_P((PGM_P)cmd); i++) {
    // write each character
    writeChar(static_cast<char>(pgm_read_byte((const char*)cmd + i)));
  }
  setState(SDI12_LISTENING);  // listen for reply
}

// This function sets up for a response to a separate data recorder by sending out a
// marking and then sending out the characters of resp one by one (for slave-side use,
// that is, when the Arduino itself is acting as an SDI-12 device rather than a
// recorder).
void SDI12::sendResponse(String& resp) {
  setState(SDI12_TRANSMITTING);       // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);        // marking is LOW
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < resp.length(); i++) {
    writeChar(resp[i]);  // write each character
  }
  setState(SDI12_LISTENING);  // return to listening state
}

void SDI12::sendResponse(const char* resp) {
  setState(SDI12_TRANSMITTING);       // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);        // marking is LOW
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen(resp); i++) {
    writeChar(resp[i]);  // write each character
  }
  setState(SDI12_LISTENING);  // return to listening state
}

void SDI12::sendResponse(FlashString resp) {
  setState(SDI12_TRANSMITTING);       // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);        // marking is LOW
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen_P((PGM_P)resp); i++) {
    // write each character
    writeChar(static_cast<char>(pgm_read_byte((const char*)resp + i)));
  }
  setState(SDI12_LISTENING);  // return to listening state
}


/* ================ Interrupt Service Routine =======================================*/

// Passes off responsibility for the interrupt to the active object.
// On espressif boards (ESP8266 and ESP32), the ISR must be stored in IRAM
#if defined(ESP32) || defined(ESP8266)
void ICACHE_RAM_ATTR SDI12::handleInterrupt() {
  if (_activeObject) _activeObject->receiveISR();
}
#else
void SDI12::handleInterrupt() {
  if (_activeObject) _activeObject->receiveISR();
}
#endif

// Creates a blank slate of bits for an incoming character
void SDI12::startChar() {
  rxState = 0x00;  // 0b00000000, got a start bit
  rxMask  = 0x01;  // 0b00000001, bit mask, lsb first
  rxValue = 0x00;  // 0b00000000, RX character to be, a blank slate
}  // startChar

// The actual interrupt service routine
void SDI12::receiveISR() {
  // time of this data transition (plus ISR latency)
  sdi12timer_t thisBitTCNT = READTIME;

  uint8_t pinLevel = digitalRead(_dataPin);  // current RX data level

  // Check if we're ready for a start bit, and if this could possibly be it.
  if (rxState == WAITING_FOR_START_BIT) {
    // If we are waiting for a start bit and the pin is low it's not a start bit, exit
    // Inverse logic start bit = HIGH
    if (pinLevel == LOW) { return; }
    // If the pin is HIGH, this should be a start bit.
    // Thus startChar(), which sets the rxState to 0, create an empty character, and a
    // new mask with a 1 in the lowest place
    startChar();
  } else {
    // If we're not waiting for a start bit, it's because we're in the middle of an
    // incomplete character and therefore this change in the pin state must be from a
    // data, parity, or stop bit.

    // Check how many bit times have passed since the last change
    uint16_t rxBits = bitTimes((uint8_t)(thisBitTCNT - prevBitTCNT));
    // Calculate how many *data+parity* bits should be left in the current character
    //      - Each character has a total of 10 bits, 1 start bit, 7 data bits, 1 parity
    // bit, and 1 stop bit
    //      - The #rxState holds record of how many of the data + parity bits we've
    // gotten (up to 8)
    //      - We have to treat the parity bit as a data bit because we don't know its
    // state
    //      - Since we're mid character, we know the start bit is past which knocks us
    // down to 9
    //      - There will always be one left over for the stop bit, which will be LOW/1
    uint8_t bitsLeft = 9 - rxState;
    // If the number of bits passed since the last transition is more than then number
    // of bits left on the character we were working on, a new character must have
    // started.
    // This will happen if the parity bit is 1 or the last bit(s) of the character and
    // the parity bit are all 1's.
    bool nextCharStarted = (rxBits > bitsLeft);

    // Check how many data+parity bits have been sent in this frame.  This will be
    // different from the rxBits if a new character has started because of the start
    // and stop bits.
    //      - If the total number of bits in this frame is more than the number of
    // data+parity bits remaining in the character, then the number of data+parity bits
    // is equal to the number of bits remaining for the character and partiy.
    //      - If the total number of bits in this frame is less than the number of data
    // bits left for the character and parity, then the number of data+parity bits
    // received in this frame is equal to the total number of bits received in this
    // frame.
    // translation:
    //    if nextCharStarted then bitsThisFrame = bitsLeft
    //                       else bitsThisFrame = rxBits
    uint8_t bitsThisFrame = nextCharStarted ? bitsLeft : rxBits;
    // Tick up the rxState by the number of data+parity bits received in the frame
    rxState += bitsThisFrame;

    // Set all the bits received between the last change and this change
    if (pinLevel == HIGH) {
      // If the current state is HIGH (and it just became so), then all bits between
      // the last change and now must have been LOW.
      // back fill previous bits with 1's (inverse logic - LOW = 1)
      while (bitsThisFrame-- > 0) {
        // for each of the bits that happened in this frame

        rxValue |= rxMask;     // Add a 1 to the LSB/right-most place of our character
                               // value from the mask
        rxMask = rxMask << 1;  // Shift the 1 in the mask up by one position
      }
      // And shift the 1 in the mask up by one more position for the current bit.
      // It's HIGH/0 now, so we don't use `|=` with the mask for this last one.
      rxMask = rxMask << 1;
    } else {
      // If the current state is LOW (and it just became so), then this bit is LOW
      // but all bits between the last change and now must have been HIGH

      // pinLevel==LOW
      // previous bits were 0's so only this bit is a 1 (inverse logic - LOW = 1)
      rxMask = rxMask << (bitsThisFrame -
                          1);  // Shift the 1 in the mask up by the number of bits past
      rxValue |= rxMask;  //  And add that shifted one to the character being created
    }

    // If this was the 8th or more bit then the character and parity are complete.
    if (rxState > 7) {
      rxValue &= 0x7F;        // Throw away the parity bit (and with 0b01111111)
      charToBuffer(rxValue);  // Put the finished character into the buffer


      // if this is LOW, or we haven't exceeded the number of bits in a
      // character (but have gotten all the data bits) then this should be a
      // stop bit and we can start looking for a new start bit.
      if ((pinLevel == LOW) || !nextCharStarted) {
        rxState = WAITING_FOR_START_BIT;  // DISABLE STOP BIT TIMER
      } else {
        // If we just switched to HIGH, or we've exceeded the total number of
        // bits in a character, then the character must have ended with 1's/LOW,
        // and this new 0/HIGH is actually the start bit of the next character.
        startChar();
      }
    }
  }
  prevBitTCNT = thisBitTCNT;  // finally remember time stamp of this change!
}

// Put a new character in the buffer
void SDI12::charToBuffer(uint8_t c) {
  // Check for a buffer overflow. If not, proceed.
  if ((_rxBufferTail + 1) % SDI12_BUFFER_SIZE == _rxBufferHead) {
    _bufferOverflow = true;
  } else {
    // Save the character, advance buffer tail.
    _rxBuffer[_rxBufferTail] = c;
    _rxBufferTail            = (_rxBufferTail + 1) % SDI12_BUFFER_SIZE;
  }
}

// Define AVR interrupts
// Check if the various interrupt vectors are defined.  If they are the ISR is
// instructed to call handleInterrupt() when they trigger.

#if defined __AVR__  // Only AVR processors use interrupts like this

#ifdef SDI12_EXTERNAL_PCINT
// Client code must call SDI12::handleInterrupt() in PCINT handler for the data pin
#else

#if defined(PCINT0_vect)
ISR(PCINT0_vect) {
  SDI12::handleInterrupt();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect) {
  SDI12::handleInterrupt();
}
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect) {
  SDI12::handleInterrupt();
}
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect) {
  SDI12::handleInterrupt();
}
#endif

#endif  // SDI12_EXTERNAL_PCINT

#endif  // __AVR__
