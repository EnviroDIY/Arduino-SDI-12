/* ======================== Arduino SDI-12 =================================

Arduino library for SDI-12 communications to a wide variety of environmental
sensors. This library provides a general software solution, without requiring
any additional hardware.

======================== Attribution & License =============================

Copyright (C) 2013  Stroud Water Research Center
Available at https://github.com/EnviroDIY/Arduino-SDI-12

Authored initially in August 2013 by:

        Kevin M. Smith (http://ethosengineering.org)
        Inquiries: SDI12@ethosengineering.org

Modified 2017 by Manuel Jimenez Buendia to work with ARM based processors
(Arduino Zero)

Maintenance and merging 2017 by Sara Damiano

based on the SoftwareSerial library (formerly NewSoftSerial), authored by:
        ladyada (http://ladyada.net)
        Mikal Hart (http://www.arduiniana.org)
        Paul Stoffregen (http://www.pjrc.com)
        Garrett Mace (http://www.macetech.com)
        Brett Hagman (http://www.roguerobotics.com/)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


================== Notes on Various Arduino-Type Processors ====================

This library requires the use of pin change interrupts (PCINT).

Not all Arduino boards have the same pin capabilities.
The known compatibile pins for common variants are shown below.

Arduino Uno: 	All pins.
Arduino Mega or Mega 2560:
10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62),
A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69).

Arduino Leonardo:
8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)

Arduino Zero:
Any pin except 4
*/

#ifndef SDI12_h
#define SDI12_h

    //  Import Required Libraries
#include <inttypes.h>           // integer types library
#include <Arduino.h>            // Arduino core library
#include <Stream.h>             // Arduino Stream library

typedef const __FlashStringHelper *FlashString;

class SDI12 : public Stream
{
protected:
  int peekNextDigit();            // override of Stream equivalent to allow custom TIMEOUT
private:
  static SDI12 *_activeObject;    // static pointer to active SDI12 instance
  void setState(uint8_t state);   // sets the state of the SDI12 objects
  void wakeSensors();             // used to wake up the SDI12 bus
  void writeChar(uint8_t out);    // used to send a char out on the data line
  void receiveChar();             // used by the ISR to grab a char from data line

  static const char * getStateName(uint8_t state);     // get state name (in ASCII)

  #ifndef __AVR__
    static uint8_t parity_even_bit(uint8_t v);
  #endif

  uint8_t _dataPin;               // reference to the data pin
  bool _bufferOverflow;           // buffer overflow status

public:
  int TIMEOUT;
  SDI12(uint8_t dataPin);        // constructor
  ~SDI12();                      // destructor
  void begin();                  // enable SDI-12 object
  void end();                    // disable SDI-12 object

  void forceHold();                     // sets line state to HOLDING
  void forceListen();                   // sets line state to LISTENING
  void sendCommand(String &cmd);        // sends the String cmd out on the data line
  void sendCommand(const char *cmd);    // sends the String cmd out on the data line
  void sendCommand(FlashString cmd);    // sends the String cmd out on the data line
  void sendResponse(String &resp);      // sends the String resp out on the data line (for slave use)
  void sendResponse(const char *resp);  // sends the String resp out on the data line (for slave use)
  void sendResponse(FlashString resp);  // sends the String resp out on the data line (for slave use)

  int available();            // returns the number of bytes available in buffer
  int peek();                 // reveals next byte in buffer without consuming
  int read();                 // returns next byte in the buffer (consumes)
  void clearBuffer();         // clears the buffer
  void flush(){};             // Waits for sending to finish - because no TX buffering, does nothing
  virtual size_t write(uint8_t byte){return 1;}  // dummy function required to inherit from Stream

  bool setActive();         // set this instance as the active SDI-12 instance
  bool isActive();          // check if this instance is active

  static void handleInterrupt();  // intermediary used by the ISR
  // #define SDI12_EXTERNAL_PCINT  // uncomment to use your own PCINT ISRs
};

#endif
