[//]: # ( @page rx_page Creating a Character - Stepping through the Rx ISR )
# Creating a Character - Stepping through the Rx ISR

[//]: # ( @tableofcontents )

[//]: # ( Start GitHub Only )
- [Creating a Character - Stepping through the Rx ISR](#creating-a-character---stepping-through-the-rx-isr)
  - [How a Character Looks in SDI-12](#how-a-character-looks-in-sdi-12)
  - [Static Variables we Need](#static-variables-we-need)
  - [Following the Mask](#following-the-mask)
    - [Waiting for a Start Bit](#waiting-for-a-start-bit)
    - [The Start of a Character](#the-start-of-a-character)
    - [The Interrupt Fires!](#the-interrupt-fires)
    - [Bit by Bit](#bit-by-bit)
      - [A LOW/1 Bit](#a-low1-bit)
      - [A HIGH/0 Bit](#a-high0-bit)
      - [Shifting Up](#shifting-up)
    - [A Finished Character](#a-finished-character)
  - [The Full Interrupt Function](#the-full-interrupt-function)

[//]: # ( End GitHub Only )

Here we'll walk step-by-step through how the SDI-12 library (and NeoSWSerial) create a character from the ISR.
Unlike SoftwareSerial which listens for a start bit and then halts all program and other ISR execution until the end of the character, this library grabs the time of the interrupt, does some quick math, and lets the processor move on.
The logic of creating a character this way is harder for a person to follow, but it pays off because we're not tieing up the processor in an ISR that lasts for 8.33ms for each character.
[10 bits @ 1200 bits/s]
For a person, that 8.33ms is trivial, but for even a "slow" 8MHz processor, that's over 60,000 ticks sitting idle per character.

So, let's look at what's happening.

[//]: # ( @section rx_specs How a Character Looks in SDI-12 )
## How a Character Looks in SDI-12

First we need to keep in mind the specifications of SDI-12:
- We use *inverse logic* that means a "1" bit is at LOW level and a "0" bit is HIGH level.
- characters are sent as 10 bits
  - 1 start bit, which is always a 0/HIGH
  - 7 data bits
  - 1 parity bit
  - 1 stop bit, which is always 1/LOW

[//]: # ( @section rx_vars Static Variables we Need )
## Static Variables we Need

And lets remind ourselves of the static variables we're using to store states:
- `prevBitTCNT` stores the time of the previous RX transition in micros
- `rxState` tracks how many bits are accounted for on an incoming character.
  - if 0: indicates that we got a start bit
  - if >0: indicates the number of bits received
- `WAITING-FOR-START-BIT` is a mask for the rxState while waiting for a start bit, it's set to 0b11111111
- `rxMask` is a bit mask for building a received character
  - The mask has a single bit set, in the place of the active bit based on the rxState
- `rxValue` is the value of the character being built

[//]: # ( @section rx_mask Following the Mask )
## Following the Mask

[//]: # ( @subsection rx_mask_wait Waiting for a Start Bit )
### Waiting for a Start Bit

The `rxState`, `rxMask`, and `rxValue` all work together to form a character.
When we're waiting for a start bit `rxValue` is empty, `rxMask` has only the bottom bit set, and `rxState` is set to WAITING-FOR-START-BIT:

```
    rxValue: |     0   0   0   0   0   0   0   0
-------------|-----------------------------------
     rxMask: |     0   0   0   0   0   0   0   1
    rxState: |     1   1   1   1   1   1   1   1
```


[//]: # ( @subsection rx_mask_start The Start of a Character )
### The Start of a Character

After we get a start bit, the `startChar()` function creates a blank slate for the new character, so our values are:

```
    rxValue: |     0   0   0   0   0   0   0   0
-------------|-----------------------------------
     rxMask: |     0   0   0   0   0   0   0   1
    rxState: |     0   0   0   0   0   0   0   0
```


[//]: # ( @subsection rx_mask_fire The Interrupt Fires! )
### The Interrupt Fires!

When an interrupts is received, we use capture the time if the interrupt in `thisBitTCNT`.
Then we subtract `prevBitTCNT` from `thisBitTCNT` and use the `bitTimes()` function to calculate how many bit-times have passed between this interrupt and the previous one.
(There's also a fudge factor in this calculation we call the [rxWindowWidth](https://github.com/SlashDevin/NeoSWSerial/pull/13#issuecomment-315463522).)


[//]: # ( @subsection rx_mask_bit Bit by Bit )
### Bit by Bit

For **each bit time that passed**, we apply the `rxMask` to the `rxValue`.
- Keep in mind multiple bit times can pass between interrupts - this happens any time there are two (or more) high or low bits in a row.
- We also leave time for the (high) start and (low) stop bit, but do anything with the `rxState`, `rxMask`, or `rxValue` for those bits.


[//]: # ( @subsubsection rx_mask_low A LOW/1 Bit )
#### A LOW/1 Bit

- if the data bit received is LOW (1) we do an `|=` (bitwise OR) between the `rxMask` and the `rxValue`

```
    rxValue: |     0   0   0   0   0   0   0   1
-------------|---------------------------------^- bit-wise or puts the one
     rxMask: |     0   0   0   0   0   0   0   1      from the rxMask into
    rxState: |     0   0   0   0   0   0   0   0      the rxValue
```


[//]: # ( @subsubsection rx_mask_high A HIGH/0 Bit )
#### A HIGH/0 Bit

- if the data bit received is HIGH (0) we do nothing

```
    rxValue: |     0   0   0   0   0   0   0   0
-------------|---------------------------------x- nothing happens
     rxMask: |     0   0   0   0   0   0   0   1
    rxState: |     0   0   0   0   0   0   0   0
```


[//]: # ( @subsubsection rx_mask_shift Shifting Up )
#### Shifting Up

- *After* applying the mask, we push everything over one bit to the left.
The top bit falls off.
  - we always add a 1 on the `rxState`, to indicate the bit arrived
  - we always add a 0 on the `rxMask` and the `rxValue`
  - the values of the second bit of the `rxValue` (?) depends on what we did in the step above

```
    rxValue: |     0        <--- | 0   0   0   0   0   0   ?   0 <--- add a zero
-------------|-------------------|---------------------------|---
     rxMask: |     0        <--- | 0   0   0   0   0   0   1   0 <--- add a zero
    rxState: |     0        <--- | 0   0   0   0   0   0   0   1 <--- add a one
-------------|-------------------|---------------------------|---
             | falls off the top |                           | added to the bottom
```


[//]: # ( @subsection rx_mask_fin A Finished Character )
### A Finished Character

After 8 bit times have passed, we should have a fully formed character with 8 bits of data (7 of the character + 1 parity).
The `rxMask`  will have the one in the top bit.
And the rxState will be filled - which just happens to be the value of `WAITING-FOR-START-BIT` for the next character.

```
    rxValue: |     ?   ?   ?   ?   ?   ?   ?   ?
-------------|-----------------------------------
     rxMask: |     1   0   0   0   0   0   0   0
    rxState: |     1   1   1   1   1   1   1   1
```


[//]: # ( @section rx_fxn The Full Interrupt Function )
## The Full Interrupt Function

Understanding how the masking creates the character, you should now be able to follow the full interrupt function below.

```cpp
// Creates a blank slate of bits for an incoming character
void SDI12::startChar() {
  rxState = 0x00;  // 0b00000000, got a start bit
  rxMask  = 0x01;  // 0b00000001, bit mask, lsb first
  rxValue = 0x00;  // 0b00000000, RX character to be, a blank slate
}  // startChar

// The actual interrupt service routine
void SDI12::receiveISR() {
  // time of this data transition (plus ISR latency)
  sdi12timer-t thisBitTCNT = READTIME;

  uint8-t pinLevel = digitalRead(-dataPin);  // current RX data level

  // Check if we're ready for a start bit, and if this could possibly be it.
  if (rxState == WAITING-FOR-START-BIT) {
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
    uint16-t rxBits = bitTimes((uint8-t)(thisBitTCNT - prevBitTCNT));
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
    uint8-t bitsLeft = 9 - rxState;
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
    uint8-t bitsThisFrame = nextCharStarted ? bitsLeft : rxBits;
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
        rxState = WAITING-FOR-START-BIT;  // DISABLE STOP BIT TIMER
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
void SDI12::charToBuffer(uint8-t c) {
  // Check for a buffer overflow. If not, proceed.
  if ((-rxBufferTail + 1) % SDI12-BUFFER-SIZE == -rxBufferHead) {
    -bufferOverflow = true;
  } else {
    // Save the character, advance buffer tail.
    -rxBuffer[-rxBufferTail] = c;
    -rxBufferTail            = (-rxBufferTail + 1) % SDI12-BUFFER-SIZE;
  }
}
```