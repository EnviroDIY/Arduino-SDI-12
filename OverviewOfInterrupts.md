[//]: # ( @page interrupts_page Overview of Interrupts )
# Overview of Interrupts

[//]: # ( @tableofcontents )

[//]: # ( Start GitHub Only )
- [Overview of Interrupts](#overview-of-interrupts)
  - [What is an Interrupt?](#what-is-an-interrupt)
  - [Directly Controlling Interrupts on an AVR Board](#directly-controlling-interrupts-on-an-avr-board)
    - [Some Vocabulary:](#some-vocabulary)
    - [Enabling an Interrupt](#enabling-an-interrupt)
    - [Disabling an Interrupt](#disabling-an-interrupt)

[//]: # ( End GitHub Only )

[//]: # ( @section interrupts_what What is an Interrupt? )
## What is an Interrupt?
An interrupt is a signal that causes the microcontroller to halt execution of the program, and perform a subroutine known as an interrupt handler or Interrupt Service Routine (ISR).
After the ISR, program execution continues where it left off.
This allows the microcontroller to efficiently handle a time-sensitive function such as receiving a burst of data on one of its pins, by not forcing the microcontroller to wait for the data.
It can perform other tasks until the interrupt is called.

All processors running some sort of Arduino core can support multiple type of interrupts.
This library specifically makes us of pin change interrupts - interrupts that trigger an ISR on any change of state detected for a specified pin.

Obviously, we don't want the processor to be halting operation every time any pin changes voltage, so we can disable or enable interrupts on a pin-by-pin basis.
For Atmel SAMD or Espressif processors the processor has dedicated control registers for each pin and the Arduino core provides us with a handy "attachInterrupt" function to use to tie our ISR to that pin.
For AVR processors, like the Arduino Uno or the EnviroDIY Mayfly, we have to use a get a bit fancier to control the interrupts.

[//]: # ( @section interrupts_avr Directly Controlling Interrupts on an AVR Board )
## Directly Controlling Interrupts on an AVR Board

[//]: # ( @subsection interrupts_vocab Some Vocabulary )
### Some Vocabulary:

**Registers**: small 1-byte (8-bit) stores of memory directly accessible by processor
PCMSK0, PCMSK1, PCMSK2, PCMSK3

**PCICRx**: a register where the three least significant bits enable or disable pin change interrupts on a range of pins
- i.e. {0,0,0,0,0,PCIE2,PCIE1,PCIE0}, where PCIE2 maps to PCMSK2, PCIE1 maps to PCMSK1, and PCIE0 maps to PCMSK0.

**PCMSKx**: a register that stores the state (enabled/disabled) of pin change interrupts on a single pin
- Each bit stores a 1 (enabled) or 0 (disabled).

On an Arduino Uno:
- There is on PCICR register controlling three ranges of pins
- There are three mask registers (PCMSK0, PCMSK1, and PCMSK2) controlling individual pins.
- Looking at one mask register, PCMSK0:
    - the 8 bits represent: PCMSK0 {PCINT7, PCINT6, PCINT5, PCINT4, PCINT3, PCINT2, PCINT1, PCINT0}
    - these map to:         PCMSK0 {XTAL2,  XTAL1,  Pin 13, Pin 12, Pin 11, Pin 10, Pin 9,  Pin 8}

**noInterrupts()**: a function to globally disable interrupts (of all types)

**interrupts()**: a function to globally enable interrupts (of all types)
- interrupts will only occur if the requisite registers are set (e.g. PCMSK and PCICR).

[//]: # ( @subsection interrupts_enable Enabling an Interrupt )
### Enabling an Interrupt

Initially, no interrupts are enabled, so PCMSK0 looks like: `{00000000}`.
If we were to use pin 9 as the data pin, we would set the bit in the pin 9 position to 1, like so: `{00000010}`.

To accomplish this, we can make use of some predefined macros.

One macro `digitalPinToPCMSK` is defined in "pins_arduino.h" which allows us to quickly get the proper register (PCMSK0, PCMSK1, or PCMSK2) given the number of the Arduino pin.
So when we write: `digitalPinToPCMSK(9)`, the address of PCMSK0 is returned.
We can use the dereferencing operator '\*' to get the value of the address.

That is, `*digitalPinToPCMSK(9)` returns: `{00000000}`.

Another macro , `digitalPinToPCMSKbit` is also defined in "pins_arduino.h" and returns the position of the bit of interest within the PCMSK of interest.

So when we write: `digitalPinToPCMSKbit(9)`, the value returned is 1, because pin 9 is represented by the "1st bit" within PCMSK0, which has a zero-based index.
That is: PCMSK { 7th bit, 6th bit, 5th bit, 4th bit, 3rd bit, 2nd bit, 1st bit, 0th bit }

The leftward bit shift operator `<<` can then used to create a "mask".
Masks are data that are used during bitwise operations to manipulate (enable / disable) one or more individual bits simultaneously.

The syntax for the operator is (variable << number_of_bits).
Some examples:

```cpp
byte a = 5;       // binary: a =  00000101

byte b = a << 4;  // binary: b =  01010000
```

So when we write: `(1<<digitalPinToPCMSKbit(9))`, we get: `{00000010}`.

Or equivalently:  `(1<<1)`, we get: `{00000010}`.

To use the mask to set the bit of interest we use the bitwise or operator `|`.
We will use the compact `|=` notation which does the operation and then stores the result back into the left hand side.


So the operation:

```cpp
*digitalPinToPCMSK(_dataPin) |= (1<<digitalPinToPCMSKbit(9));
```
Accomplishes:

```
    (1<<digitalPinToPCMSKbit(9))              {00000010}

    PCMSK0                               |    {00000000}

                                             -------------

                                              {00000010}
```


We must also enable the global control for the interrupt. This is done in a similar fashion:

`*digitalPinToPCICR(_dataPin) |= (1<<digitalPinToPCICRbit(_dataPin));`


Now let's assume that part of your Arduino sketch outside of SDI-12 had set a pin change interrupt on pin 13.
Pin 9 and pin 13 are on the same PCMSK in the case of the Arduino Uno.

This time before we set the bit for pin nine,

`*digitalPinToPCMSK(9)` returns: `{00100000}`.

So now:

```
    (1<<digitalPinToPCMSKbit(9))              {00000010}

    PCMSK0                               |    {00100000}

                                            -------------

                                              {00100010}
```

By using a bitmask and bitwise operation, we have successfully enabled pin 9 without effecting the state of pin 13.


[//]: # ( @subsection interrupts_disable Disabling an Interrupt )
### Disabling an Interrupt

When the we would like to put the SDI-12 object in the DISABLED state, (e.g. the destructor is called), we need to make sure the bit corresponding to the data pin is unset.

Let us consider again the case of where an interrupt has been enabled on pin 13: {00100010}.
We want to be sure not to disturb this interrupt when disabling the interrupt on pin 9.

We will make use of similar macros, but this time we will use an inverted bit mask and the AND operation.
The `&=` operator is equivalent to a bitwise AND operation between the PCMSK register of interest and the previous result, which is then stored back into the PCMSK of interest.
The `*` is the dereferencing operator, which is equivalently translated to "value pointed by", and allows us to store the result back into the proper PCMSK.

Again `1<<digitalPinToPCMSKbit(9)` returns `{00000010}`

The inversion symbol `~` modifies the result to `{11111101}`

So to finish our example:

```
    ~(1<<digitalPinToPCMSKbit(9))             {11111101}

    PCMSK0                               &    {00100010}

                                            -------------

                                              {00100000}
```

So only the interrupt on pin 13 remains set.
As a matter of book keeping, if we unset the last bit in the PCMSK, we ought to also unset the respective bit in the PCICR.

    `!(*digitalPinToPCMSK(9)`

        will evaluate TRUE if PCMSK {00000000}

        will evaluate FALSE if PCMSK != {00000000}

In this case, pin 13 is set, so the expression would be FALSE.
If we go back to the original case without pin 13, the expression after disabling pin 9 would evaluate to TRUE.

Therefore if we evaluate to TRUE, we should tidy up:
```cpp
if(!*digitalPinToPCMSK(_dataPin)){
      *digitalPinToPCICR(_dataPin) &= ~(1<<digitalPinToPCICRbit(_dataPin));
  }
```
