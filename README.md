Arduino-SDI-12
==============

Arduino library for SDI-12 communications to a wide variety of environmental sensors. This library provides a general software solution, without requiring any additional hardware, to implement the SDI-12 communication protocol between an Arduino-based data logger and SDI-12-enabled sensors.

[SDI-12](http://www.sdi-12.org/) is an asynchronous, ASCII, serial communications protocol that was developed for intelligent sensory instruments that typically monitor environmental data. [Advantages of SDI-12](http://en.wikipedia.org/wiki/SDI-12) include the ability to use a single available data channel for many sensors.

This work is motivated by the [EnviroDIY community](http://envirodiy.org/) vision to create an open source hardware and software stack to deliver near real time environmental data from wireless sensor networks, such as the Arduino-compatible [EnviroDIY™ Mayfly Data Logger](http://envirodiy.org/mayfly/).

## Getting Started

Learn more, below, about this library's:
* [Origins and Inherited Limitations](https://github.com/EnviroDIY/Arduino-SDI-12#origins-and-inherited-limitations);
* [Compatibility Considerations](https://github.com/EnviroDIY/Arduino-SDI-12#compatibility-considerations);
* [Variants and Branches](https://github.com/EnviroDIY/Arduino-SDI-12#variants-and-branches) we created to overcome some limitations.

Try running our [Example sketches](https://github.com/EnviroDIY/Arduino-SDI-12/tree/master/examples) with your Arduino board and SDI-12 sensor.

Dive into the details of how this library works by reading the documentation in our [Arduino-SDI-12 wiki](https://github.com/StroudCenter/Arduino-SDI-12/wiki).  [Note:  this is currently somewhat out of date.  Sorry!]


## Origins and Inherited Limitations

This library was developed from the [SoftwareSerial](https://github.com/arduino/Arduino/tree/master/hardware/arduino/avr/libraries/SoftwareSerial) library that is a built-in [standard Arduino library](https://www.arduino.cc/en/Reference/Libraries). It was further modified to use a timer to improve read stability and decrease the amount of time universal interrupts are disabled using logic from [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial).

The most obvious "limitation" is that this library will conflict with all other libraries that make use of pin change interrupts.  You will be unable to compile them together.  Some other libraries using pin change interrupts include [SoftwareSerial](https://github.com/arduino/Arduino/tree/master/hardware/arduino/avr/libraries/SoftwareSerial), [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial), [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt/), [PinChangeInt](https://playground.arduino.cc/Main/PinChangeInt), [Servo](https://www.arduino.cc/en/Reference/Servo), and quite a number of other libraries.  See the notes under [Variants and Branches](https://github.com/EnviroDIY/Arduino-SDI-12#variants-and-branches) below for advice in using this library in combination with such libraries.

Another non-trivial, but hidden limitation is that _all_ interrupts are disabled during most of the transmission of each character, which can interfere with other processes. That includes other pin-change interrupts, clock/timer interrupts, external interrupts, and every other type of processor interrupt.  This is particularly problematic for Arduino-SDI-12, because SDI-12 operates at a very slow baud rate (only 1200 baud). This translates to ~8.3 mS of "radio silence" from the processor for each character that goes out via SDI-12, which adds up to ~380-810ms per command!  Interrupts are enabled for the majority of the time while the processor is listening for responses.

For most AVR boards, this library will also conflict with the [tone](https://www.arduino.cc/reference/en/language/functions/advanced-io/tone/) function because of its utilization of timer 2.  There will be no obvious compile error, but because SDI-12 and the tone library may use different clock-prescaler functions, the results for both might be rather unexpected.  All 8MHz AVR boards will also have unresolvable prescaler conflicts with [NeoSWSerial](https://github.com/SlashDevin/NeoSWSerial).  The pre-scaler values needed for the SDI-12 functionality are set in the begin() function and reset to their original values in the end() function.

## Compatibility Considerations

This library has been tested with an Arduino Uno (AtMega328p), EnviroDIY Mayfly (AtMega1284p), Adafruit Feather 32u4 (AtMega32u4, identical to Arduino Leonardo), and an Adafruit Feather M0 (SAMD21G18, identical to Arduino Zero).  It should also work on an Arduino Mega (AtMega2560), Gemma/AtTiny board, and most other AVR processors  running on the Arduino framework.

The Arduino Due, Arduino 101, Teensy, and ESP8266/ESP32 boards are not supported at this time.  If you are interested in adding support for those boards, please send pull requests.

Not all data pins are available for use with this Arduino-SDI-12 library. Pin availability depends on the micro-controller. These pins will work on those processors:

* **AtMega328p / Arduino Uno:** 	All pins
* **AtMega1284p / EnviroDIY Mayfly:**  All pins
* **ATmega2560 / Arduino Mega or Mega 2560:** 10, 11, 12, 13, 14, 15, 50, 51, 52, 53, A8 (62), A9 (63), A10 (64), A11 (65), A12 (66), A13 (67), A14 (68), A15 (69)
* **AtMega32u4 / Arduino Leonardo or Adafruit Feather:** 8, 9, 10, 11, 14 (MISO), 15 (SCK), 16 (MOSI)
* **SAMD21G18 / Arduino Zero:**  All pins (except 4 on the zero)

Note that not all of these pins are available with our [Variants and Branches](https://github.com/EnviroDIY/Arduino-SDI-12#variants-and-branches), below.


## Variants and Branches
As we've described, the default "master" branch of this library will conflict with SoftwareSerial and any other library that monopolizes all pin change interrupt vectors for all AVR boards.  To allow simultaneous use of Arduino-SDI-12 and SoftwareSerial, we have created additional variants of these libraries that we maintain as separate branches of this repository. For background information, my be helpful to read our [Overview of Interrupts](https://github.com/EnviroDIY/Arduino-SDI-12/wiki/2b.-Overview-of-Interrupts) wiki page or this [Arduino Pin Change Interrupts article](https://thewanderingengineer.com/2014/08/11/arduino-pin-change-interrupts/).

#### EnviroDIY_SDI12
EnviroDIY_SDI12 is the default master branch of this repository. It controls and monopolizes all pin change interrupt vectors, and can therefore have conflicts with any variant of SoftwareSerial and other libraries that use interrupts.

#### EnviroDIY_SDI12_PCINT3
EnviroDIY_SDI12_PCINT3 is in the Mayfly branch of this repository, and was historically was called "SDI12_mod".  It's been cropped to only control interrupt vector 3, or PCINT3 (D), which on the Mayfly (or Sodaq Mbili) corresponds to Pins D0-D7.
It is designed to be compatible with [EnviroDIY_SoftwareSerial_PCINT12](https://github.com/EnviroDIY/SoftwareSerial_PCINT12) library (which controls interrupt vectors PCINT1 (B) & PCINT2 (C) / Mayfly pins D08-D15 & D16-D23) and [EnviroDIY PcInt PCINT0](https://github.com/EnviroDIY/PcInt_PCINT0) (which controls interrupt vectors PCINT0 (A) / Mayfly pins D24-D31/A0-A7).
Note that different AtMega1284p boards have a different mapping from the physical PIN numbers to the listed digital PIN numbers that are printed on the board. One of the most helpful lists of pins and interrupts vectors is in the the [Pin/Port Bestiary wiki page for the Enable Interrupt library](https://github.com/GreyGnome/EnableInterrupt/wiki/Usage#PIN__PORT_BESTIARY).

#### EnviroDIY_SDI12_ExtInts
EnviroDIY_SDI12_ExtInts is the ExtInt branch of this repository. It doesn't control any of the interrupts, but instead relies on an external interrupt management library (like [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt)) to assign the SDI-12 receive data function to the right pin.  This is the least stable because there's some extra delay because the external library is involved, but the use of timers in the SDI-12 library greatly increases it's stability.  It's also the easiest to get working in combination with any other pin change interrupt based library. It can be paired with the [EnviroDIY_SoftwareSerial_ExtInts](https://github.com/EnviroDIY/SoftwareSerial_ExternalInts) libraries (which is, by the way, extremely unstable).

If you would like to use a different pin change interrupt library, uncomment the line ```#define SDI12_EXTERNAL_PCINT``` in SDI12.h and recompile the library.  Then, in your own code call `SDI12::handleInterrupt()` as the interrupt for the SDI12 pin using the other interrupt library.  Example j shows doing this in GreyGnome's [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt) library.


## Contribute
Open an [issue](https://github.com/EnviroDIY/Arduino-SDI-12/issues) to suggest and discuss potential changes/additions.

For power contributors:

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D


## License
The SDI12 library code is released under the GNU Lesser Public License (LGPL 2.1) -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE) file for details.

Example Arduino sketches are released under the BSD 3-Clause License -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE.md) file for details.

Documentation is licensed as [Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/) (CC-BY-SA) copyright.

## Credits
[EnviroDIY](http://envirodiy.org/)™ is presented by the Stroud Water Research Center, with contributions from a community of enthusiasts sharing do-it-yourself ideas for environmental science and monitoring.

[Kevin M. Smith](https://github.com/Kevin-M-Smith) was the primary developer of the Arduino-SDI-12 library, with input from [S. Hicks](https://github.com/s-hicks2) and [Anthony Aufdenkampe](https://github.com/aufdenkampe).

[Sara Damiano](https://github.com/SRGDamia1) is now the primary maintainer, with input from many [other contributors](https://github.com/EnviroDIY/Arduino-SDI-12/graphs/contributors).

This project has benefited from the support from the following funders:

* National Science Foundation, awards [EAR-0724971](http://www.nsf.gov/awardsearch/showAward?AWD_ID=0724971), [EAR-1331856](http://www.nsf.gov/awardsearch/showAward?AWD_ID=1331856), [ACI-1339834](http://www.nsf.gov/awardsearch/showAward?AWD_ID=1339834)
* William Penn Foundation, grant 158-15
* Stroud Water Research Center endowment
