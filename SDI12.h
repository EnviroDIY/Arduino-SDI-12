/* ======================== Arduino SDI-12 =================================

Arduino library for SDI-12 communications to a wide variety of environmental 
sensors. This library provides a general software solution, without requiring 
any additional hardware.

======================== Attribution & License =============================

Copyright (C) 2013  Stroud Water Research Center
Available at https://github.com/StroudCenter/Arduino-SDI-12

Authored initially in August 2013 by:

        Kevin M. Smith (http://ethosengineering.org)
        Inquiries: SDI12@ethosengineering.org

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
#include <avr/interrupt.h>      // interrupt handling
#include <avr/parity.h>         // optimized parity bit handling
#include <inttypes.h>			// integer types library
#include <Arduino.h>            // Arduino core library
#include <Stream.h>				// Arduino Stream library

class SDI12 : public Stream
{
protected:
  int peekNextDigit();			// override of Stream equivalent to allow custom TIMEOUT
private:
  static SDI12 *_activeObject;	// static pointer to active SDI12 instance
  void setState(uint8_t state); // sets the state of the SDI12 objects
  void wakeSensors();			// used to wake up the SDI12 bus
  void writeChar(uint8_t out); 	// used to send a char out on the data line
  void receiveChar();			// used by the ISR to grab a char from data line
  
public:
  int TIMEOUT;
  SDI12(uint8_t dataPin);		// constructor
  ~SDI12();						// destructor
  void begin();					// enable SDI-12 object
  void end();					// disable SDI-12 object
  
  void forceHold(); 			// sets line state to HOLDING
  void sendCommand(String cmd);	// sends the String cmd out on the data line
    
  int available();			// returns the number of bytes available in buffer
  int peek();				// reveals next byte in buffer without consuming
  int read();				// returns next byte in the buffer (consumes)
  void flush();				// clears the buffer 
  virtual size_t write(uint8_t byte){}; // dummy function required to inherit from Stream

  bool setActive(); 		// set this instance as the active SDI-12 instance
  bool isActive();			// check if this instance is active

  static inline void handleInterrupt(); // intermediary used by the ISR
};

#endif
