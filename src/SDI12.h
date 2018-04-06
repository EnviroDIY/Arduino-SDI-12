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
*/

#ifndef SDI12_h
#define SDI12_h

    //  Import Required Libraries
#include <inttypes.h>           // integer types library
#include <Arduino.h>            // Arduino core library
#include <Stream.h>             // Arduino Stream library

typedef const __FlashStringHelper *FlashString;

#define NO_IGNORE_CHAR '\x01' // a char not found in a valid ASCII numeric field
#define SDI12_BUFFER_SIZE 64   // max Rx buffer size

class SDI12 : public Stream
{
protected:
  // hides the version from the stream to allow custom timeout value
  int peekNextDigit(LookaheadMode lookahead, bool detectDecimal);

private:

  // For the various SDI12 states
  typedef enum SDI12_STATES
  {
    DISABLED = 0,
    ENABLED = 1,
    HOLDING = 2,
    TRANSMITTING = 3,
    LISTENING = 4
  } SDI12_STATES;

  static SDI12 *_activeObject;    // static pointer to active SDI12 instance

  void setPinInterrupts(bool enable);  // Turns pin interrupts on or off
  void setState(SDI12_STATES state);   // sets the state of the SDI12 objects
  void wakeSensors();             // used to wake up the SDI12 bus
  void writeChar(uint8_t out);    // used to send a char out on the data line
  void startChar();               // creates a blank slate for a new incoming character
  void receiveISR();              // the actual function responding to changes in rx line state
  void charToBuffer(uint8_t c);   // puts a finished character into the SDI12 buffer

  #ifndef __AVR__
    static uint8_t parity_even_bit(uint8_t v);
  #endif

  uint8_t _dataPin;               // reference to the data pin

  static uint8_t _rxBuffer[SDI12_BUFFER_SIZE];  // A single buffer for ALL SDI-12 objects
  static volatile uint8_t _rxBufferTail;
  static volatile uint8_t _rxBufferHead;
  bool _bufferOverflow;           // buffer overflow status

public:
  SDI12();                          // constructor - without argument, for better library integration
  SDI12(uint8_t dataPin);           // constructor
  ~SDI12();                         // destructor
  void begin();                     // enable SDI-12 object
  void begin(uint8_t dataPin);      // enable SDI-12 object - if you use the empty constuctor, USE THIS
  void end();                       // disable SDI-12 object
  int TIMEOUT;                      // value to return if a parse times out
  void setTimeoutValue(int value);  // sets the value to return if a parse int or parse float times out
  uint8_t getDataPin();             // returns the data pin for the current instace

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
  virtual size_t write(uint8_t byte);  // standard stream function

  // hide the Stream equivalents to allow custom value to be returned on timeout
  long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR);
  float parseFloat(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR);

  bool setActive();         // set this instance as the active SDI-12 instance
  bool isActive();          // check if this instance is active

  static void handleInterrupt();  // intermediary used by the ISR

  // #define SDI12_EXTERNAL_PCINT  // uncomment to use your own PCINT ISRs

};

#endif  // SDI12_h
