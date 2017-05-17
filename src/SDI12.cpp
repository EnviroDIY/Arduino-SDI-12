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


==================== Notes on SDI-12, Specification v1.3 ======================

Overview:

SDI-12 is a communications protocol that uses a single data wire to
communicate with up to 62 uniquely addressed sensors. So long as each
sensor supports SDI-12, mixed sensor types can appear on the same data
bus. Each address is a single character. The valid ranges are 0-9, a-z,
and A-Z. Only the datalogger can initiate communications on the data
bus.

It does so by pulling the data line into a 5v state for at least 12
milliseconds to wake up all the sensors, before returning the line into
a 0v state for 8 milliseconds announce an outgoing command. The command
contains both the action to be taken, and the address of the device who
should respond. If there is a sensor on the bus with that address, it is
responsible for responding to the command. Sensors should ignore
commands that were not issued to them, and should return to a sleep
state until the datalogger again issues the wakeup sequence.

Physical Connections:            1 data line (0v - 5.5v)
    1 12v power line (9.6v - 16v)
    1 ground line

Baud Rate:                         1200 bits per second

Data Frame Format:                10 bits per data frame
    1 start bit
    7 data bits (least significant bit first)
    1 even parity bit
    1 stop bit

Data Line:                         SDI-12 communication uses a single
    bi-directional data line
    with three-state, inverse logic.

    LINE CONDITION    |  BINARY STATE | VOLTAGE RANGE
    -----------------------------------------------
    marking                1              -0.5 to 1.0 volts
    spacing                0               3.5 to 5.5 volts
    transition          undefined          1.0 to 3.5 volts

      _____       _____       _____       _____       _____     spacing
5v   |     |     |     |     |     |     |     |     |     |
     |  0  |  1  |  0  |  1  |  0  |  1  |  0  |  1  |  0  | transition
Ov___|     |_____|     |_____|     |_____|     |_____|     |___ marking


For more information, and for a list of commands and responses, please see
SDI-12.org, official site of the SDI-12 Support Group.


==================== Code Organization ======================
0.    Includes, Defines, & Variable Declarations
1.    Buffer Setup
2.    Data Line States, Overview of Interrupts
3.    Constructor, Destructor, SDI12.begin(), and SDI12.end()
4.    Waking up, and talking to, the sensors.
5.    Reading from the SDI-12 object. available(), peek(), read(), clearBuffer()
6.    Using more than one SDI-12 object, isActive() and setActive().
7.    Interrupt Service Routine (getting the data into the buffer)

=========== 0. Includes, Defines, & Variable Declarations =============

0.1 - Include the header file for this library.
0.2 - defines the size of the buffer
0.3 - defines value for DISABLED state (see section 2)
0.4 - defines value for ENABLED state (not used, reserved for future)
0.5 - defines value for DISABLED state (see section 2)
0.6 - defines value for TRANSMITTING state (see section 2)
0.7 - defines value for LISTENING state
0.8 - defines value for the spacing of bits.
    1200 bits per second implies 833 microseconds per bit.
    830 seems to be a reliable value given the overhead of the call.
0.9    - holds a custom value that indicates a
    TIMEOUT has occurred from parseInt() or parseFloat(). This should not be set to
    a possible data value.

0.10 - a static pointer to the active object. See section 6.
0.11 - a reference to the data pin, used throughout the library
0.12 - holds the buffer overflow status

*/

#include "SDI12.h"                   // 0.1 header file for this library

#define _BUFFER_SIZE 64              // 0.2 max RX buffer size
#define DISABLED 0                   // 0.3 value for DISABLED state
#define ENABLED 1                    // 0.4 value for ENABLED state
#define HOLDING 2                    // 0.5 value for DISABLED state
#define TRANSMITTING 3               // 0.6 value for TRANSMITTING state
#define LISTENING 4                  // 0.7 value for LISTENING state
#define SPACING 830                  // 0.8 bit timing in microseconds
int TIMEOUT = -9999;                 // 0.9 value to return to indicate TIMEOUT

SDI12 *SDI12::_activeObject = NULL;  // 0.10 pointer to active SDI12 object


/* =========== 1. Buffer Setup ============================================

The buffer is used to store characters from the SDI-12 data line.
Characters are read into the buffer when an interrupt is received on the
data line. The buffer uses a circular implementation with pointers to
both the head and the tail. There is one buffer per instance of the
SDI-12 object. This will consume a good deal of RAM, so be prudent in
running multiple instances.

For more information on circular buffers:
http://en.wikipedia.org/wiki/Circular_buffer

1.1 - Define a maximum buffer size (in number of characters). Increasing
the buffer size will use more RAM. If you exceed 256 characters, be sure
to change the data type of the index to support the larger range of
addresses.

1.2 - Create a character array of the specified size. 1.3 - Index to
buffer head. (unsigned 8-bit integer, can map from 0-255) 1.4 - Index to
buffer tail. (unsigned 8-bit integer, can map from 0-255)

*/

// See section 0 above.           // 1.1 - max buffer size
char _rxBuffer[_BUFFER_SIZE];     // 1.2 - buff for incoming
uint8_t _rxBufferHead = 0;        // 1.3 - index of buff head
uint8_t _rxBufferTail = 0;        // 1.4 - index of buff tail


/* =========== 2. Data Line States ===============================

The Arduino is responsible for managing communication with the sensors.
Since all the data transfer happens on the same line, the state of the
data line is very important.

When the pin is in the HOLDING state, it is holding the line LOW so that
interference does not unintentionally wake the sensors up. The interrupt
is disabled for the dataPin, because we are not expecting any SDI-12
traffic. In the TRANSMITTING state, we would like exclusive control of
the Arduino, so we shut off all interrupts, and vary the voltage of the
dataPin in order to wake up and send commands to the sensor. In the
LISTENING state, we are waiting for a sensor to respond, so we drop the
voltage level to LOW and relinquish control (INPUT). If we would like to
disable all SDI-12 functionality, then we set the system to the DISABLED
state, removing the interrupt associated with the dataPin. For
predictability, we set the pin to a LOW level high impedance state
(INPUT).

State                    Interrupts        Pin Mode    Pin Level
HOLDING                  Pin Disable        OUTPUT        LOW
TRANSMITTING             All Disable        OUTPUT      VARYING
LISTENING                All Enable         INPUT         LOW
DISABLED                 Pin Disable        INPUT         LOW

------------------------------|  Sequencing |------------------------------

Generally, this is acceptable.
HOLDING --> TRANSMITTING --> LISTENING --> TRANSMITTING --> LISTENING -->

If you have interference, you should force a hold, using forceHold();
HOLDING --> TRANSMITTING --> LISTENING --> done reading, forceHold(); HOLDING

-------------------------|  Function Descriptions |-------------------------

2.1 - Sets up parity and interrupts for different processor types - that is,
imports the interrupts and parity for the AVR processors where they exist.

2.2 - A function to return a string with the current line state.

2.3 - Sets the proper state. This is a private function, and only used
internally. It uses #define values of HOLDING, TRANSMITTING, LISTENING,
and DISABLED to determine which state should be set. The grid above
defines the settings applied in changing to each state.

2.4 - A public function which forces the line into a "holding" state.
This is generally unneeded, but for deployments where interference is an
issue, it should be used after all expected bytes have been returned
from the sensor.

2.5 - A public function which forces the line into a "listening" state.
This may be needed for implementing a slave-side device, which should
relinquish control of the data line when not transmitting.
*/


// 2.1 - Processor specific parity and interrupts
#if defined __AVR__
  #include <avr/interrupt.h>      // interrupt handling
  #include <util/parity.h>        // optimized parity bit handling
#else
// Added MJB: parity fuction to replace the one specific for AVR from util/parity.h
// look for a better optimized code in
// http://www.avrfreaks.net/forum/easy-method-calculate-even-parity-16-bit
uint8_t SDI12::parity_even_bit(uint8_t v)
{
  uint8_t parity = 0;
  while (v)
  {
    parity = !parity;
    v = v & (v - 1);
  }
  return parity;
}
#endif

// 2.2 - Returns the current state
const char *SDI12::getStateName(uint8_t state)
{
    const char * retval = "UNKNOWN";
    if (state == HOLDING) {
        retval = "HOLDING";
    }
    else if (state == TRANSMITTING) {
        retval = "TRANSMITTING";
    }
    else if (state == LISTENING) {
        retval = "LISTENING";
    }
    else if (state == ENABLED) {
        retval = "ENABLED";
    }
    else if (state == DISABLED) {
        retval = "DISABLED";
    }
    return retval;
}

// 2.3 - sets the state of the SDI-12 object.
void SDI12::setState(uint8_t state){
  if(state == HOLDING){
    pinMode(_dataPin,INPUT);  // added to make output work after pinMode to OUTPUT (don't know why, but works)
    pinMode(_dataPin,OUTPUT);
    digitalWrite(_dataPin,LOW);
    #if defined __AVR__
      *digitalPinToPCMSK(_dataPin) &= ~(1<<digitalPinToPCMSKbit(_dataPin));
    #else
      detachInterrupt(digitalPinToInterrupt(_dataPin));
    #endif
    return;
  }
  if(state == TRANSMITTING){
    pinMode(_dataPin,INPUT);  // added to make output work after pinMode to OUTPUT (don't know why, but works)
    pinMode(_dataPin,OUTPUT);
    noInterrupts();             // supplied by Arduino.h, same as cli()
    return;
  }
  if(state == LISTENING) {
    digitalWrite(_dataPin,LOW);
    pinMode(_dataPin,INPUT);
    interrupts();                // supplied by Arduino.h, same as sei()
    #if defined __AVR__
      *digitalPinToPCICR(_dataPin) |= (1<<digitalPinToPCICRbit(_dataPin));
      *digitalPinToPCMSK(_dataPin) |= (1<<digitalPinToPCMSKbit(_dataPin));
    #else
      attachInterrupt(digitalPinToInterrupt(_dataPin),handleInterrupt, CHANGE);
    #endif
  }
  else {                         // implies state==DISABLED
      digitalWrite(_dataPin,LOW);
      pinMode(_dataPin,INPUT);
      #if defined __AVR__
        *digitalPinToPCMSK(_dataPin) &= ~(1<<digitalPinToPCMSKbit(_dataPin));
        if(!*digitalPinToPCMSK(_dataPin)){
            *digitalPinToPCICR(_dataPin) &= ~(1<<digitalPinToPCICRbit(_dataPin));
        }
      #else
        detachInterrupt(digitalPinToInterrupt(_dataPin));
      #endif
  }
}

// 2.4 - forces a HOLDING state.
void SDI12::forceHold(){
    setState(HOLDING);
}

// 2.5 - forces a LISTENING state.
void SDI12::forceListen(){
    setState(LISTENING);
}

/* ======= 3. Constructor, Destructor, SDI12.begin(), and SDI12.end()  =======

3.1 - The constructor requires a single parameter: the pin to be used
for the data line. When the constructor is called it resets the buffer
overflow status to FALSE and assigns the pin number "dataPin" to the
private variable "_dataPin".

3.2 - When the destructor is called, it's main task is to disable any
interrupts that had been previously assigned to the pin, so that the pin
will behave as expected when used for other purposes. This is achieved
by putting the SDI-12 object in the DISABLED state.

3.3 - This is called to begin the functionality of the SDI-12 object. It
has no parameters as the SDI-12 protocol is fully specified (e.g. the
baud rate is set). It sets the object as the active object (if multiple
SDI-12 instances are being used simultaneously).

3.4 - This can be called to temporarily cease all functionality of the
SDI-12 object. It is not as harsh as destroying the object with the
destructor, as it will maintain the memory buffer.

*/

//  3.1 Constructor
SDI12::SDI12(uint8_t dataPin){
  _bufferOverflow = false;
  _dataPin = dataPin;
}

//  3.2 Destructor
SDI12::~SDI12(){ setState(DISABLED); }

//  3.3 Begin
void SDI12::begin(){
  // setState(HOLDING);
  setActive();
}

//  3.4 End
void SDI12::end() { setState(DISABLED); }


/* ============= 4. Waking up, and talking to, the sensors. ===================

4.1 - wakeSensors() literally wakes up all the sensors on the bus. The
SDI-12 protocol requires a pulse of HIGH voltage for at least 12
milliseconds followed immediately by a pulse of LOW voltage for at least
8.3 milliseconds. The values here are close to those values, but provide
100 extra microseconds of wiggle room. Setting the SDI-12 object into
the TRANSMITTING allows us to assert control of the line without
triggering any interrupts.

4.2 - This function writes a character out to the data line. SDI-12
specifies the general transmission format of a single character as:

    10 bits per data frame
    1 start bit
    7 data bits (least significant bit first)
    1 even parity bit
    1 stop bit

We also recall that we are using inverse logic, so HIGH represents 0,
and LOW represents a 1. If you are unclear on any of these terms, I
would recommend that you look them up before proceeding. They will be
better explained elsewhere.

The transmission takes several steps.
The variable name for the outgoing character is "out".

+ 4.2.1 - Determine the proper even parity bit (will an additional 1 or 0
make the final number of 1's even?)

First we grab the bit using an optimized macro from parity.h
    parity_even_bit(out)

Then we bit shift it into the proper place, which is the most
significant bit position, since the characters we are using are only
7-bits.
    (parity_even_bit(out)<<7);

Then we use the '|=' operator to set the bit if necessary.

+ 4.2.2 - Send the start bit. The start bit is always a '0', so we simply
write the dataPin HIGH for SPACING microseconds.

+ 4.2.3 - Send the payload (the 7 character bits and the parity bit) least
significant bit first. This is accomplished bitwise AND operations on a
moving mask (00000001) --> (00000010) --> (00000100)... and so on. This
functionality makes use of the '<<=' operator which stores the result of
the bit-shift back into the left hand side.

If the result of (out & mask) determines whether a 1 or 0 should be sent.
Again, here inverse logic may lead to easy confusion.

if(out & mask){
      digitalWrite(_dataPin, LOW);
    }
    else{
      digitalWrite(_dataPin, HIGH);
    }

+ 4.2.4 - Send the stop bit. The stop bit is always a '1', so we simply
write the dataPin LOW for SPACING microseconds.

4.3 - sendCommand(String cmd) is a publicly accessible function that
wakes sensors and sends out a String byte by byte the command line.

4.4 - sendResponse(String resp) is a publicly accessible function that
sends out an 8.33 ms marking and a String byte by byte the command line.
This is needed if the Arduino is acting as an SDI-12 device itself, not as a
recorder for another SDI-12 device

*/

// 4.1 - this function wakes up the entire sensor bus
void SDI12::wakeSensors(){
  setState(TRANSMITTING);
  digitalWrite(_dataPin, HIGH);
  delayMicroseconds(12100);  // Required break of 12 milliseconds
  // delayMicroseconds(12500);  // Required break of 12 milliseconds
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(8400);  // Required marking of 8.33 milliseconds
  // delayMicroseconds(8800);  // Required marking of 8.33 milliseconds
}

// 4.2 - this function writes a character out on the data line
void SDI12::writeChar(uint8_t out)
{
  out |= (parity_even_bit(out)<<7);          // 4.2.1 - parity bit

  digitalWrite(_dataPin, HIGH);              // 4.2.2 - start bit
  delayMicroseconds(SPACING);

  for (byte mask = 0x01; mask; mask<<=1){    // 4.2.3 - send payload
    if(out & mask){
      digitalWrite(_dataPin, LOW);
    }
    else{
      digitalWrite(_dataPin, HIGH);
    }
    delayMicroseconds(SPACING);
  }

  digitalWrite(_dataPin, LOW);                // 4.2.4 - stop bit
  delayMicroseconds(SPACING);
}

//    4.3    - this function sends out the characters of the String cmd, one by one
void SDI12::sendCommand(String &cmd)
{
  wakeSensors();             // wake up sensors
  for (int unsigned i = 0; i < cmd.length(); i++){
    writeChar(cmd[i]);       // write each character
  }
  setState(LISTENING);       // listen for reply
}

void SDI12::sendCommand(const char *cmd)
{
  wakeSensors();             // wake up sensors
  for (int unsigned i = 0; i < strlen(cmd); i++){
    writeChar(cmd[i]);      // write each character
  }
  setState(LISTENING);      // listen for reply
}

void SDI12::sendCommand(FlashString cmd)
{
  wakeSensors();            // wake up sensors
  for (int unsigned i = 0; i < strlen_P((PGM_P)cmd); i++){
    writeChar((char)pgm_read_byte((const char *)cmd + i));  // write each character
  }
  setState(LISTENING);      // listen for reply
}

//  4.4 - this function sets up for a response to a separate data recorder by
//        sending out a marking and then sending out the characters of resp
//        one by one (for slave-side use, that is, whdn the Arduino itself is
//        acting as an SDI-12 device rather than a recorder).
void SDI12::sendResponse(String &resp)
{
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(8330);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < resp.length(); i++){
    writeChar(resp[i]);     // write each character
  }
  setState(LISTENING);      // return to listening state
}

void SDI12::sendResponse(const char *resp)
{
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(8330);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen(resp); i++){
    writeChar(resp[i]);     // write each character
  }
  setState(LISTENING);      // return to listening state
}

void SDI12::sendResponse(FlashString resp)
{
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(8330);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen_P((PGM_P)resp); i++){
    writeChar((char)pgm_read_byte((const char *)resp + i));  // write each character
  }
  setState(LISTENING);      // return to listening state
}

/* ============= 5. Reading from the SDI-12 object.  ===================

5.1 - available() is a public function that returns the number of
characters available in the buffer.

To understand how:
_rxBufferTail + _BUFFER_SIZE - _rxBufferHead) % _BUFFER_SIZE;
accomplishes this task, we will use a few examples.

To start take the buffer below that has _BUFFER_SIZE = 10. The
message "abc" has been wrapped around (circular buffer).

_rxBufferTail = 1 // points to the '-' after c
_rxBufferHead = 8 // points to 'a'

[ c ] [ - ] [ - ] [ - ] [ - ] [ - ] [ - ] [ - ]  [ a ] [ b ]

The number of available characters is (1 + 10 - 8) % 10 = 3

The '%' or modulo operator finds the remainder of division of one number
by another. In integer arithmetic 3 / 10 = 0, but has a remainder of 3.
We can only get the remainder by using the the modulo '%'. 3 % 10 = 3.
This next case demonstrates more clearly why the modulo is used.

_rxBufferTail = 4 // points to the '-' after c
_rxBufferHead = 1 // points to 'a'

[ a ] [ b ] [ c ] [ - ] [ - ] [ - ] [ - ] [ - ]  [ - ] [ - ]

The number of available characters is (4 + 10 - 1) % 10 = 3

If we did not use the modulo we would get either ( 4 + 10 - 1 ) = 13
characters or ( 4 + 10 - 1 ) / 10 = 1 character. Obviously neither is
correct.

If there has been a buffer overflow, available() will return -1.

5.2 - peek() is a public function that allows the user to look at the
character that is at the head of the buffer. Unlike read() it does not
consume the character (i.e. the index addressed by _rxBufferHead is not
changed). peek() returns -1 if there are no characters to show.

5.3 - clearBuffer() is a public function that clears the buffers contents by
setting the index for both head and tail back to zero.

5.4 - read() returns the character at the current head in the buffer
after incrementing the index of the buffer head. This action 'consumes'
the character, meaning it can not be read from the buffer again. If you
would rather see the character, but leave the index to head intact, you
should use peek();

5.5 - peekNextDigit() is called by the Stream class. It is overridden
here to allow for a custom TIMEOUT value. The default value for the
Stream class is to return 0. This makes distinguishing timeouts from
true zero readings impossible. Therefore the default value has been
set to -9999 in section 0 of the code. It is a public variable and
can be changed dynamically within a program by calling:
    mySDI12.TIMEOUT = (int) newValue

*/

// 5.1 - reveals the number of characters available in the buffer
int SDI12::available()
{
  if(_bufferOverflow) return -1;
  return (_rxBufferTail + _BUFFER_SIZE - _rxBufferHead) % _BUFFER_SIZE;
}

// 5.2 - reveals the next character in the buffer without consuming
int SDI12::peek()
{
  if (_rxBufferHead == _rxBufferTail) return -1;  // Empty buffer? If yes, -1
  return _rxBuffer[_rxBufferHead];                // Otherwise, read from "head"
}

// 5.3 - a public function that clears the buffer contents and
// resets the status of the buffer overflow.
void SDI12::clearBuffer()
{
  _rxBufferHead = _rxBufferTail = 0;
  _bufferOverflow = false;
}

// 5.4 - reads in the next character from the buffer (and moves the index ahead)
int SDI12::read()
{
  _bufferOverflow = false;                             // Reading makes room in the buffer
  if (_rxBufferHead == _rxBufferTail) return -1;       // Empty buffer? If yes, -1
  uint8_t nextChar = _rxBuffer[_rxBufferHead];         // Otherwise, grab char at head
  _rxBufferHead = (_rxBufferHead + 1) % _BUFFER_SIZE;  // increment head
  return nextChar;                                     // return the char
}

// 5.5 - this function is called by the Stream class when parsing digits
int SDI12::peekNextDigit()
{
  int c;
  while (1) {
    c = timedPeek();
    if (c < 0) return TIMEOUT; // timeout
    if (c == '-') return c;
    if (c >= '0' && c <= '9') return c;
    read(); // discard non-numeric
  }
}

/* ============= 6. Using more than one SDI-12 object.  ===================

This library is allows for multiple instances of itself running on
different pins, however, you should use care. The instances DO NOT share
a buffer, as they do in the multi-instance case for SoftwareSerial, so
it will consume more RAM than you might expect.

SDI-12 can support up to 62 sensors on a single pin/bus, so it is not
necessary to use an instance for each sensor.

Because we are using pin change interrupts there can only be one active
object at a time (since this is the only reliable way to determine which
pin the interrupt occurred on). The active object is the only object
that will respond properly to interrupts. However promoting another
instance to Active status does not automatically remove the interrupts
on the other pin. For proper behavior it is recommended to use this
pattern:

    mySDI12.forceHold();
    myOtherSDI12.setActive();

Other notes: Promoting an object into the Active state will set it as
HOLDING. See 6.1 for more information.

Calling mySDI12.begin() will assert mySDI12 as the new active object,
until another instance calls myOtherSDI12.begin() or
myOtherSDI12.setActive().

Calling mySDI12.end() does NOT hand-off active status to another SDI-12
instance.

You can check on the active object by calling mySDI12.isActive(), which
will return a boolean value TRUE if active or FALSE if inactive.

6.1 - a method for setting the current object as the active object.
returns TRUE if the object was not formerly the active object and now
is. returns

Promoting an inactive to the active instance will start it in the
HOLDING state and return TRUE.

Otherwise, if the object is currently the active instance, it will
remain unchanged and return FALSE.

6.2 - a method for checking if the object is the active object. Returns
true if the object is currently the active object, false otherwise.

*/

// 6.1 - a method for setting the current object as the active object
bool SDI12::setActive()
{
  if (_activeObject != this)
  {
    setState(HOLDING);
    _activeObject = this;
    return true;
  }
  return false;
}

// 6.2 - a method for checking if this object is the active object
bool SDI12::isActive() { return this == _activeObject; }


/* ============== 7. Interrupt Service Routine  ===================

We have received an interrupt signal, what should we do?

7.1 - Passes off responsibility for the interrupt to the active object.

7.2 - This function quickly reads a new character from the data line in
to the buffer. It takes place over a series of key steps.

+ 7.2.1 - Check for the start bit. If it is not there, interrupt may be
from interference or an interrupt we are not interested in, so return.

+ 7.2.2 - Make space in memory for the new character "newChar".

+ 7.2.3 - Wait half of a SPACING to help center on the next bit. It will
not actually be centered, or even approximately so until
delayMicroseconds(SPACING) is called again.

+ 7.2.4 - For each of the 8 bits in the payload, read wether or not the
line state is HIGH or LOW. We use a moving mask here, as was previously
demonstrated in the writeByte() function.

The loop runs from i=0x1 (hexadecimal notation for 00000001) to i<0x80
(hexadecimal notation for 10000000). So the loop effectively uses the
masks following masks: 00000001
00000010
00000100
00001000
00010000
00100000
01000000 and their inverses.

Here we use an if / else structure that helps to balance the time it
takes to either a HIGH vs a LOW, and helps maintain a constant timing.

+ 7.2.5 - Skip the parity bit. There is no error checking.

+ 7.2.6 - Skip the stop bit.

+ 7.2.7 - Check for an overflow. We do this by checking if advancing the
tail would make it have the same index as the head (in a circular
fashion).

+ 7.2.8 - Save the byte into the buffer if there has not been an
overflow, and then advance the tail index.

7.3 - Check if the various interrupt vectors are defined. If they are
the ISR is instructed to call _handleInterrupt() when they trigger. */

// 7.1 - Passes off responsibility for the interrupt to the active object.
void SDI12::handleInterrupt(){
  if (_activeObject) _activeObject->receiveChar();
}

// 7.2 - Quickly reads a new character into the buffer.
void SDI12::receiveChar()
{
  if (digitalRead(_dataPin))                // 7.2.1 - Start bit?
  {
    uint8_t newChar = 0;                    // 7.2.2 - Make room for char.

    delayMicroseconds(SPACING/2);           // 7.2.3 - Wait 1/2 SPACING

    for (uint8_t i=0x1; i<0x80; i <<= 1)    // 7.2.4 - read the 7 data bits
    {
      delayMicroseconds(SPACING);
      uint8_t noti = ~i;
      if (!digitalRead(_dataPin))
        newChar |= i;
      else
        newChar &= noti;
    }

    delayMicroseconds(SPACING);              // 7.2.5 - Skip the parity bit.
    delayMicroseconds(SPACING);              // 7.2.6 - Skip the stop bit.

    // 7.2.7 - Overflow? If not, proceed.
    if ((_rxBufferTail + 1) % _BUFFER_SIZE == _rxBufferHead)
    { _bufferOverflow = true;
    } else {                                 // 7.2.8 - Save char, advance tail.
      _rxBuffer[_rxBufferTail] = newChar;
      _rxBufferTail = (_rxBufferTail + 1) % _BUFFER_SIZE;
    }
  }
}

//7.3
#if defined __AVR__

#ifdef SDI12_EXTERNAL_PCINT
  // Client code must call SDI12::handleInterrupt() in PCINT handler for the data pin
#else

#if defined(PCINT0_vect)
ISR(PCINT0_vect){ SDI12::handleInterrupt(); }
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect){ SDI12::handleInterrupt(); }
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect){ SDI12::handleInterrupt(); }
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect){ SDI12::handleInterrupt(); }
#endif

#endif  // SDI12_EXTERNAL_PCINT
#endif  // __AVR__
