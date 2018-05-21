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

Arduino Uno:  All pins.
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

Physical Connections:  1 data line (0v - 5.5v)
                       1 12v power line (9.6v - 16v)
                       1 ground line

Baud Rate:  1200 bits per second

Data Frame Format:  10 bits per data frame
                    1 start bit
                    7 data bits (least significant bit first)
                    1 stop bit

Data Line:  SDI-12 communication uses a single bi-directional data line with
three-state, inverse logic.

LINE CONDITION  |  BINARY STATE  |  VOLTAGE RANGE
---------------------------------------------------
   marking              1        -0.5 to 1.0 volts
   spacing              0         3.5 to 5.5 volts
  transition        undefined     1.0 to 3.5 volts

      _____       _____       _____       _____       _____     spacing
5v   |     |     |     |     |     |     |     |     |     |
     |  0  |  1  |  0  |  1  |  0  |  1  |  0  |  1  |  0  | transition
Ov___|     |_____|     |_____|     |_____|     |_____|     |___ marking


For more information, and for a list of commands and responses, please see
SDI-12.org, official site of the SDI-12 Support Group.


==================== Code Organization ======================
0.  Includes, Defines, & Variable Declarations
1.  Buffer Setup
2.  Reading from the SDI-12 buffer
3.  Constructor, Destructor, SDI12.begin(), and SDI12.end()
4.  Using more than one SDI-12 object, isActive() and setActive()
5.  Setting proper data Line States
6.  Waking up, and talking to, the sensors
7.  Interrupt Service Routine (getting the data into the buffer)
*/

/*=========== 0. Includes, Defines, & Variable Declarations =============
*/

#include "SDI12.h"                   // Header file for this library
#include "SDI12_boards.h"

SDI12 *SDI12::_activeObject = NULL;  // Pointer to active SDI12 object

static const uint16_t bitWidth_micros = (uint16_t) 833;  // The size of a bit in microseconds
    // 1200 baud = 1200 bits/second ~ 833.333 µs/bit
static const uint16_t lineBreak_micros = (uint16_t) 12100;  // The required "break" before sending commands
    // break >= 12ms
static const uint16_t marking_micros = (uint16_t) 8330;  // The required mark before a command or response
    // marking >= 8.33ms

static const uint8_t txBitWidth = TICKS_PER_BIT;
static const uint8_t rxWindowWidth = RX_WINDOW_FUDGE;  // A fudge factor to make things work
static const uint8_t bitsPerTick_Q10 = BITS_PER_TICK_Q10;
static const uint8_t WAITING_FOR_START_BIT = 0xFF;  // 0b11111111

static uint16_t prevBitTCNT;     // previous RX transition in micros
static uint8_t rxState;          // 0: got start bit; >0: bits rcvd
static uint8_t rxMask;           // bit mask for building received character
static uint8_t rxValue;          // character being built

// static method for getting a 16-bit value from 2 8-bit values
static uint16_t mul8x8to16(uint8_t x, uint8_t y)
{return x*y;}

// static method for calculating the number of bit-times that have elapsed
static uint16_t bitTimes( uint8_t dt )
{
  return mul8x8to16( dt + rxWindowWidth, bitsPerTick_Q10 ) >> 10;
} // bitTimes


/* =========== 1. Buffer Setup ============================================

The buffer is used to store characters from the SDI-12 data line.
Characters are read into the buffer when an interrupt is received on the
data line. The buffer uses a circular implementation with pointers to
both the head and the tail. There is one buffer per instance of the
SDI-12 object. This will consume a good deal of RAM, so be prudent in
running multiple instances.

For more information on circular buffers:
http://en.wikipedia.org/wiki/Circular_buffer

1.1 - Initialize a single buffer for all SDI12 objects. Increasing
the buffer size will use more RAM. If you exceed 256 characters, be sure
to change the data type of the index to support the larger range of
addresses.
1.2 - Index to buffer head. (unsigned 8-bit integer, can map from 0-255)
1.3 - Index to buffer tail. (unsigned 8-bit integer, can map from 0-255)

*/

uint8_t SDI12::_rxBuffer[SDI12_BUFFER_SIZE];  // 1.1 - buff for incoming
volatile uint8_t SDI12::_rxBufferTail = 0;    // 1.2 - index of buff head
volatile uint8_t SDI12::_rxBufferHead = 0;    // 1.3 - index of buff tail

/* =========== 2. Reading from the SDI-12 buffer  ==========================

2.1 - available() is a public function that returns the number of
characters available in the buffer.

To understand how:
_rxBufferTail + SDI12_BUFFER_SIZE - _rxBufferHead) % SDI12_BUFFER_SIZE;
accomplishes this task, we will use a few examples.

To start take the buffer below that has SDI12_BUFFER_SIZE = 10. The
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

2.2 - peek() is a public function that allows the user to look at the
character that is at the head of the buffer. Unlike read() it does not
consume the character (i.e. the index addressed by _rxBufferHead is not
changed). peek() returns -1 if there are no characters to show.

2.3 - clearBuffer() is a public function that clears the buffers contents by
setting the index for both head and tail back to zero.

2.4 - read() returns the character at the current head in the buffer
after incrementing the index of the buffer head. This action 'consumes'
the character, meaning it can not be read from the buffer again. If you
would rather see the character, but leave the index to head intact, you
should use peek();

2.5 - peekNextDigit(), parseInt(), and parseFloat() are functions in the Stream
class.  Although they are not virtual and cannot be "overridden," recreting
them here "hides" the stream default versions to allow for a custom timeout
return value. The default value for the Stream class is to return 0. This makes
distinguishing timeouts from true zero readings impossible. Therefore the
default value has been set to -9999 in the being function. The value returned by
a timeout (TIMEOUT) is a public variable and can be changed dynamically
within a program by calling:
    mySDI12.TIMEOUT = (int) newValue
or using the setTimeoutValue(int) function.

*/

// 2.1 - reveals the number of characters available in the buffer
int SDI12::available()
{
  if(_bufferOverflow) return -1;
  return (_rxBufferTail + SDI12_BUFFER_SIZE - _rxBufferHead) % SDI12_BUFFER_SIZE;
}

// 2.2 - reveals the next character in the buffer without consuming
int SDI12::peek()
{
  if (_rxBufferHead == _rxBufferTail) return -1;  // Empty buffer? If yes, -1
  return _rxBuffer[_rxBufferHead];                // Otherwise, read from "head"
}

// 2.3 - a public function that clears the buffer contents and
// resets the status of the buffer overflow.
void SDI12::clearBuffer()
{
  _rxBufferHead = _rxBufferTail = 0;
  _bufferOverflow = false;
}

// 2.4 - reads in the next character from the buffer (and moves the index ahead)
int SDI12::read()
{
  _bufferOverflow = false;                             // Reading makes room in the buffer
  if (_rxBufferHead == _rxBufferTail) return -1;       // Empty buffer? If yes, -1
  uint8_t nextChar = _rxBuffer[_rxBufferHead];         // Otherwise, grab char at head
  _rxBufferHead = (_rxBufferHead + 1) % SDI12_BUFFER_SIZE;  // increment head
  return nextChar;                                     // return the char
}

// 2.5 - these functions hide the stream equivalents to return a custom timeout value
int SDI12::peekNextDigit(LookaheadMode lookahead, bool detectDecimal)
{
  int c;
  while (1) {
    c = timedPeek();

    if( c < 0 ||
        c == '-' ||
        (c >= '0' && c <= '9') ||
        (detectDecimal && c == '.')) return c;

    switch( lookahead ){
        case SKIP_NONE: return -1; // Fail code.
        case SKIP_WHITESPACE:
            switch( c ){
                case ' ':
                case '\t':
                case '\r':
                case '\n': break;
                default: return -1; // Fail code.
            }
        case SKIP_ALL:
            break;
    }
    read();  // discard non-numeric
  }
}

long SDI12::parseInt(LookaheadMode lookahead, char ignore)
{
  bool isNegative = false;
  long value = 0;
  int c;

  c = peekNextDigit(lookahead, false);
  // ignore non numeric leading characters
  if(c < 0)
    return TIMEOUT; // TIMEOUT returned if timeout
    //  THIS IS THE ONLY DIFFERENCE BETWEEN THIS FUNCTION AND THE STREAM DEFAULT!

  do{
    if(c == ignore)
      ; // ignore this character
    else if(c == '-')
      isNegative = true;
    else if(c >= '0' && c <= '9')        // is c a digit?
      value = value * 10 + c - '0';
    read();  // consume the character we got with peek
    c = timedPeek();
  }
  while( (c >= '0' && c <= '9') || c == ignore );

  if(isNegative)
    value = -value;
  return value;
}

// the same as parseInt but returns a floating point value
float SDI12::parseFloat(LookaheadMode lookahead, char ignore)
{
  bool isNegative = false;
  bool isFraction = false;
  long value = 0;
  int c;
  float fraction = 1.0;

  c = peekNextDigit(lookahead, true);
    // ignore non numeric leading characters
  if(c < 0)
    return TIMEOUT; // TIMEOUT returned if timeout
    //  THIS IS THE ONLY DIFFERENCE BETWEEN THIS FUNCTION AND THE STREAM DEFAULT!

  do{
    if(c == ignore)
      ; // ignore
    else if(c == '-')
      isNegative = true;
    else if (c == '.')
      isFraction = true;
    else if(c >= '0' && c <= '9')  {      // is c a digit?
      value = value * 10 + c - '0';
      if(isFraction)
         fraction *= 0.1;
    }
    read();  // consume the character we got with peek
    c = timedPeek();
  }
  while( (c >= '0' && c <= '9')  || (c == '.' && !isFraction) || c == ignore );

  if(isNegative)
    value = -value;
  if(isFraction)
    return value * fraction;
  else
    return value;
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

3.5 - These set a custom value to return if a parse int or parse float function
times out.  By default this value is -9999.

*/

//  3.1 Constructor
SDI12::SDI12(){
  _bufferOverflow = false;
}
SDI12::SDI12(uint8_t dataPin){
  _bufferOverflow = false;
  _dataPin = dataPin;
}

//  3.2 Destructor
SDI12::~SDI12(){
  setState(DISABLED);
  _activeObject = NULL;
  // Set the timer prescalers back to original values
  // NOTE:  This does NOT reset SAMD board pre-scalers!
  resetSDI12TimerPrescale();
}

//  3.3 Begin
void SDI12::begin(){
  // setState(HOLDING);
  setActive();
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
  // Set up the prescaler as needed for timers
  // This function is defined in SDI12_boards.h
  configSDI12TimerPrescale();
}
void SDI12::begin(uint8_t dataPin){
  _dataPin = dataPin;
  begin();
}

//  3.4 End
void SDI12::end()
{
  setState(DISABLED);
  _activeObject = NULL;
  // Set the timer prescalers back to original values
  // NOTE:  This does NOT reset SAMD board pre-scalers!
  resetSDI12TimerPrescale();
}

//  3.5 Set the timeout return
void SDI12::setTimeoutValue(int value) { TIMEOUT = value; }

//  3.6 Return the data pin for the SDI-12 instance
uint8_t SDI12::getDataPin() { return _dataPin; }


/* ============= 4. Using more than one SDI-12 object.  ===================

This library is allows for multiple instances of itself running on the same or
different pins.  SDI-12 can support up to 62 sensors on a single pin/bus,
so it is notnecessary to use an instance for each sensor.

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
HOLDING. See 4.1 for more information.

Calling mySDI12.begin() will assert mySDI12 as the new active object,
until another instance calls myOtherSDI12.begin() or
myOtherSDI12.setActive().

Calling mySDI12.end() does NOT hand-off active status to another SDI-12
instance.

You can check on the active object by calling mySDI12.isActive(), which
will return a boolean value TRUE if active or FALSE if inactive.

4.1 - a method for setting the current object as the active object.
returns TRUE if the object was not formerly the active object and now
is. returns

Promoting an inactive to the active instance will start it in the
HOLDING state and return TRUE.

Otherwise, if the object is currently the active instance, it will
remain unchanged and return FALSE.

4.2 - a method for checking if the object is the active object. Returns
true if the object is currently the active object, false otherwise.

*/

// 4.1 - a method for setting the current object as the active object
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

// 4.2 - a method for checking if this object is the active object
bool SDI12::isActive() { return this == _activeObject; }


/* =========== 5. Data Line States ===============================

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
TRANSMITTING           All/Pin Disable      OUTPUT      VARYING
LISTENING                All Enable         INPUT         LOW
DISABLED                 Pin Disable        INPUT         LOW

------------------------------|  Sequencing |------------------------------

Generally, this is acceptable.
HOLDING --> TRANSMITTING --> LISTENING --> TRANSMITTING --> LISTENING -->

If you have interference, you should force a hold, using forceHold();
HOLDING --> TRANSMITTING --> LISTENING --> done reading, forceHold(); HOLDING

-------------------------|  Function Descriptions |-------------------------

5.1 - Sets up parity and interrupts for different processor types - that is,
imports the interrupts and parity for the AVR processors where they exist.

5.2 - A private helper function to turn pin interupts on or off

5.3 - Sets the proper state. This is a private function, and only used
internally. The grid above defines the settings applied in changing to each state.

5.4 - A public function which forces the line into a "holding" state.
This is generally unneeded, but for deployments where interference is an
issue, it should be used after all expected bytes have been returned
from the sensor.

5.5 - A public function which forces the line into a "listening" state.
This may be needed for implementing a slave-side device, which should
relinquish control of the data line when not transmitting.
*/

// 5.1 - Processor specific parity and interrupts
#if defined __AVR__
  #include <avr/interrupt.h>      // interrupt handling
  #include <util/parity.h>        // optimized parity bit handling
#else
// Added MJB: parity fuction to replace the one specific for AVR from util/parity.h
// http://graphics.stanford.edu/~seander/bithacks.html#ParityNaive
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

// 5.2 - a helper function to switch pin interrupts on or off
void SDI12::setPinInterrupts(bool enable)
{
  #ifndef SDI12_EXTERNAL_PCINT
    if (enable)
    {
      #if defined __AVR__
        *digitalPinToPCICR(_dataPin) |= (1<<digitalPinToPCICRbit(_dataPin));  // Enable interrupts on the register with the pin of interest
        *digitalPinToPCMSK(_dataPin) |= (1<<digitalPinToPCMSKbit(_dataPin));  // Enable interrupts on the specific pin of interest
        // The interrupt function is actually attached to the interrupt way down in section 7.5
      #else
        attachInterrupt(digitalPinToInterrupt(_dataPin),handleInterrupt, CHANGE);  // Merely need to attach the interrupt function to the pin
      #endif
    }
    else
    {
      #if defined __AVR__
        *digitalPinToPCMSK(_dataPin) &= ~(1<<digitalPinToPCMSKbit(_dataPin));  // Disable interrupts on the specific pin of interest
        if(!*digitalPinToPCMSK(_dataPin)){  // If there are no other pins on the register left with enabled interrupts, disable the whole register
          *digitalPinToPCICR(_dataPin) &= ~(1<<digitalPinToPCICRbit(_dataPin));
        }
        // We don't detach the function from the interrupt for AVR processors
      #else
        detachInterrupt(digitalPinToInterrupt(_dataPin));  // Merely need to detach the interrupt function from the pin
      #endif
    }
  #endif
}

// 5.3 - sets the state of the SDI-12 object.
void SDI12::setState(SDI12_STATES state){
  switch (state)
  {
    case HOLDING:
    {
      pinMode(_dataPin,INPUT);      // added to make output work after pinMode to OUTPUT (don't know why, but works)
      pinMode(_dataPin,OUTPUT);     // Pin mode = output
      digitalWrite(_dataPin,LOW);   // Pin state = low
      setPinInterrupts(false);      // Interrupts disabled on data pin
      break;
    }
    case TRANSMITTING:
    {
      pinMode(_dataPin,INPUT);   // added to make output work after pinMode to OUTPUT (don't know why, but works)
      pinMode(_dataPin,OUTPUT);  // Pin mode = output
      setPinInterrupts(false);   // Interrupts disabled on data pin
      break;
    }
    case LISTENING:
    {
      digitalWrite(_dataPin,LOW);   // Pin state = low
      pinMode(_dataPin,INPUT);      // Pin mode = input
      interrupts();                 // Enable general interrupts
      setPinInterrupts(true);       // Enable Rx interrupts on data pin
      rxState = WAITING_FOR_START_BIT;
      break;
    }
    default:  // DISABLED or ENABLED
    {
      digitalWrite(_dataPin,LOW);   // Pin state = low
      pinMode(_dataPin,INPUT);      // Pin mode = input
      setPinInterrupts(false);      // Interrupts disabled on data pin
      break;
    }
  }
}

// 5.4 - forces a HOLDING state.
void SDI12::forceHold(){
    setState(HOLDING);
}

// 5.5 - forces a LISTENING state.
void SDI12::forceListen(){
    setState(LISTENING);
}


/* ============= 6. Waking up, and talking to, the sensors. ===================

6.1 - wakeSensors() literally wakes up all the sensors on the bus. The
SDI-12 protocol requires a pulse of HIGH voltage for at least 12
milliseconds followed immediately by a pulse of LOW voltage for at least
8.3 milliseconds. Setting the SDI-12 object into the TRANSMITTING allows us to
assert control of the line without triggering any interrupts.

6.2 - This function writes a character out to the data line. SDI-12
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
The variable name for the outgoing character is "outChar".

6.3 - sendCommand(String cmd) is a publicly accessible function that
wakes sensors and sends out a String byte by byte the command line.

6.4 - sendResponse(String resp) is a publicly accessible function that
sends out an 8.33 ms marking and a String byte by byte the command line.
This is needed if the Arduino is acting as an SDI-12 device itself, not as a
recorder for another SDI-12 device

*/

// 6.1 - this function wakes up the entire sensor bus
void SDI12::wakeSensors() {
  setState(TRANSMITTING);
  // Universal interrupts can be on while the break and marking happen because
  // timings for break and from the recorder are not critical.
  // Interrupts on the pin are disabled for the entire transmitting state
  digitalWrite(_dataPin, HIGH);
  delayMicroseconds(lineBreak_micros);  // Required break of 12 milliseconds
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(marking_micros);  // Required marking of 8.33 milliseconds
}

// 6.2 - this function writes a character out on the data line
void SDI12::writeChar(uint8_t outChar) {
  uint8_t currentTxBitNum = 0; // first bit is start bit
  uint8_t bitValue = 1; // start bit is HIGH (inverse parity...)

  noInterrupts();  // _ALL_ interrupts disabled so timing can't be shifted

  uint8_t t0 = TCNTX; // start time
  digitalWrite(_dataPin, HIGH);  // immediately get going on the start bit
  // this gives us 833µs to calculate parity and position of last high bit
  currentTxBitNum++;

  uint8_t parityBit = parity_even_bit(outChar);  // Calculate the parity bit
  outChar |= (parityBit<<7);  // Add parity bit to the outgoing character

  // Calculate the position of the last bit that is a 0/HIGH (ie, HIGH, not marking)
  // That bit will be the last time-critical bit.  All bits after that can be
  // sent with interrupts enabled.

  uint8_t lastHighBit = 9;  // The position of the last bit that is a 0 (ie, HIGH, not marking)
  uint8_t msbMask = 0x80;  // A mask with all bits at 1
  while (msbMask & outChar) {
    lastHighBit--;
    msbMask >>= 1;
  }

  // Hold the line for the rest of the start bit duration
  while ((uint8_t)(TCNTX - t0) < txBitWidth) {}
  t0 = TCNTX; // advance start time

  // repeat for all data bits until the last bit different from marking
  while (currentTxBitNum++ < lastHighBit) {
    bitValue = outChar & 0x01;  // get next bit in the character to send
    if (bitValue){
      digitalWrite(_dataPin, LOW);  // set the pin state to LOW for 1's
    }
    else{
      digitalWrite(_dataPin, HIGH);  // set the pin state to HIGH for 0's
    }
    // Hold the line for this bit duration
    while ((uint8_t)(TCNTX - t0) < txBitWidth) {}
    t0 = TCNTX; // advance start time
    outChar = outChar >> 1;  // shift character to expose the following bit
  }

  // Set the line low for the all remaining 1's and the stop bit
  digitalWrite(_dataPin, LOW);

  interrupts(); // Re-enable universal interrupts as soon as critical timing is past

  // Hold the line low until the end of the 10th bit
  uint8_t bitTimeRemaining = txBitWidth*(10-lastHighBit);
  while ((uint8_t)(TCNTX - t0) < bitTimeRemaining) {}

}

// The typical write functionality for a stream object
// This allows you to use the stream print functions to send commands out on
// the SDI-12, line, but it will not wake the sensors in advance of the command.
size_t SDI12::write(uint8_t byte) {
  setState(TRANSMITTING);
  writeChar(byte);         // write the character/byte
  setState(LISTENING);       // listen for reply
  return 1;                  // 1 character sent
}

//    6.3    - this function sends out the characters of the String cmd, one by one
void SDI12::sendCommand(String &cmd) {
  wakeSensors();             // set state to transmitting and send break/marking
  for (int unsigned i = 0; i < cmd.length(); i++){
    writeChar(cmd[i]);       // write each character
  }
  setState(LISTENING);       // listen for reply
}

void SDI12::sendCommand(const char *cmd) {
  wakeSensors();             // wake up sensors
  for (int unsigned i = 0; i < strlen(cmd); i++){
    writeChar(cmd[i]);      // write each character
  }
  setState(LISTENING);      // listen for reply
}

void SDI12::sendCommand(FlashString cmd) {
  wakeSensors();            // wake up sensors
  for (int unsigned i = 0; i < strlen_P((PGM_P)cmd); i++){
    writeChar((char)pgm_read_byte((const char *)cmd + i));  // write each character
  }
  setState(LISTENING);      // listen for reply
}

//  6.4 - this function sets up for a response to a separate data recorder by
//        sending out a marking and then sending out the characters of resp
//        one by one (for slave-side use, that is, when the Arduino itself is
//        acting as an SDI-12 device rather than a recorder).
void SDI12::sendResponse(String &resp) {
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < resp.length(); i++){
    writeChar(resp[i]);     // write each character
  }
  setState(LISTENING);      // return to listening state
}

void SDI12::sendResponse(const char *resp) {
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen(resp); i++){
    writeChar(resp[i]);     // write each character
  }
  setState(LISTENING);      // return to listening state
}

void SDI12::sendResponse(FlashString resp) {
  setState(TRANSMITTING);   // Get ready to send data to the recorder
  digitalWrite(_dataPin, LOW);
  delayMicroseconds(marking_micros);  // 8.33 ms marking before response
  for (int unsigned i = 0; i < strlen_P((PGM_P)resp); i++){
    writeChar((char)pgm_read_byte((const char *)resp + i));  // write each character
  }
  setState(LISTENING);      // return to listening state
}


/* ============== 7. Interrupt Service Routine  ===================

We have received an interrupt signal, what should we do?

7.1 - Passes off responsibility for the interrupt to the active object.

7.2 - Creates a blank slate of bits for an incoming character

7.3 - This function checks which direction the change of the interrupt was and
then uses that to populate the bits of the character.

+ First check if we're expecting a start bit and if this change is in the
right direction for the start bit. If it is not, interrupt may be
from interference or an interrupt we are not interested in, so return.
Because the SDI-12 protocol specifies inverse logic, the end of a start bit will
be a change from LOW to HIGH.

+ If this isn't a start bit, and a new character has been started, figure out
where in the character we are at this change and fill out bits accordingly.

Here we use an if / else structure that helps to balance the time it
takes to either a HIGH vs a LOW, and helps maintain a constant timing.

7.4 - Puts a new character into the active SDI-12 buffer

7.5 - Check if the various interrupt vectors are defined. If they are
the ISR is instructed to call _handleInterrupt() when they trigger. */

// 7.1 - Passes off responsibility for the interrupt to the active object.
void SDI12::handleInterrupt(){
  if (_activeObject) _activeObject->receiveISR();
}

// 7.2 - Creates a blank slate of bits for an incoming character
void SDI12::startChar()
{
  rxState = 0;           // got a start bit
  rxMask  = 0x01;  // 0b00000001, bit mask, lsb first
  rxValue = 0x00;  // 0b00000000, RX character to be, a blank slate
} // startChar

// 7.3 - The actual interrupt service routine
void SDI12::receiveISR()
{
  uint8_t thisBitTCNT = TCNTX;               // time of this data transition (plus ISR latency)
  uint8_t pinLevel = digitalRead(_dataPin);  // current RX data level

  // Check if we're ready for a start bit, and if this could possibly be it
  // Otherwise, just ignore the interrupt and exit
  if (rxState == WAITING_FOR_START_BIT) {
     // If it is low it's not a start bit, exit
     // Inverse logic start bit = HIGH
    if (pinLevel == LOW) {
      return;
    }
    // If it is HIGH, this should be a start bit
    // Thus set the rxStat to 0, create an empty character, and a new mask with a 1 in the lowest place
    startChar();
  }

  // if the character is incomplete, and this is not a start bit,
  // then this change is from a data, parity, or stop bit
  else {

    // check how many bit times have passed since the last change
    // the rxWindowWidth is just a fudge factor
    uint16_t rxBits = bitTimes(thisBitTCNT - prevBitTCNT);
    // Serial.println(rxBits);
    // calculate how many *data+parity* bits should be left
    // We know the start bit is past and are ignoring the stop bit (which will be LOW/1)
    // We have to treat the parity bit as a data bit because we don't know its state
    uint8_t bitsLeft = 9 - rxState;
    // note that a new character *may* have started if more bits have been
    // received than should be left.
    // This will also happen if the parity bit is 1 or the last bit(s) of the
    // character and the parity bit are all 1's.
    bool nextCharStarted = (rxBits > bitsLeft);

    // check how many data+parity bits have been sent in this frame
    // If the total number of bits in this frame is more than the number of data+parity
    // bits remaining in the character, then the number of data+parity bits is equal
    // to the number of bits remaining for the character and partiy.  If the total
    // number of bits in this frame is less than the number of data bits left
    // for the character and parity, then the number of data+parity bits received
    // in this frame is equal to the total number of bits received in this frame.
    // translation:
    //    if nextCharStarted then bitsThisFrame = bitsLeft
    //                       else bitsThisFrame = rxBits
    uint8_t bitsThisFrame = nextCharStarted ? bitsLeft : rxBits;
    // Tick up the rxState by that many bits
    rxState += bitsThisFrame;

    // Set all the bits received between the last change and this change
    // If the current state is HIGH (and it just became so), then all bits between
    // the last change and now must have been LOW.
    if (pinLevel == HIGH) {
      // back fill previous bits with 1's (inverse logic - LOW = 1)
      while (bitsThisFrame-- > 0) {
        rxValue |= rxMask;  // Add a 1 to the LSB/right-most place
        rxMask   = rxMask << 1;  // Shift the 1 in the mask up by one position
      }
      rxMask = rxMask << 1;  // Shift the 1 in the mask up by one more position
    }
    // If the current state is LOW (and it just became so), then this bit is LOW
    // but all bits between the last change and now must have been HIGH
    else { // pinLevel==LOW
      // previous bits were 0's so only this bit is a 1 (inverse logic - LOW = 1)
      rxMask   = rxMask << (bitsThisFrame-1);  // Shift the 1 in the mask up by the number of bits past
      rxValue |= rxMask;  //  Add that shifted one to the character being created
    }

    // If this was the 8th or more bit then the character and parity are complete.
    if (rxState > 7) {
      rxValue &= 0x7F;  // 0b01111111, Throw away the parity bit
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
  prevBitTCNT = thisBitTCNT;  // remember time stamp of this change!
}

// 7.4 - Put a new character in the buffer
void SDI12::charToBuffer( uint8_t c )
{
  // Check for a buffer overflow. If not, proceed.
  if ((_rxBufferTail + 1) % SDI12_BUFFER_SIZE == _rxBufferHead)
    { _bufferOverflow = true; }
  // Save the character, advance buffer tail.
  else
  {
     _rxBuffer[_rxBufferTail] = c;
     _rxBufferTail = (_rxBufferTail + 1) % SDI12_BUFFER_SIZE;
  }
}

// 7.5 - Define AVR interrupts

#if defined __AVR__  // Only AVR processors use interrupts like this

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
