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

Dive into the details of how this library works by reading the documentation in our [Arduino-SDI-12 wiki](https://github.com/StroudCenter/Arduino-SDI-12/wiki).s


## Origins and Inherited Limitations

This library was developed from the [SoftwareSerial](https://github.com/arduino/Arduino/tree/master/hardware/arduino/avr/libraries/SoftwareSerial) library that is a built-in [standard Arduino library](https://www.arduino.cc/en/Reference/Libraries). As such, it also shares many of the [limitations of SoftwareSerial](https://www.arduino.cc/en/Reference/SoftwareSerial).

A primary limitation is that all [pin-change interrupts](https://thewanderingengineer.com/2014/08/11/arduino-pin-change-interrupts/) are disabled during transmission, which can interfere with other processes and libraries that also use interrupts. This is particularly problematic for Arduino-SDI-12, because SDI-12 operates at a very slow baud rate (only 1200 baud). This translates to ~8.3 mS of "radio silence" from the processor for each character that goes in or out via SDI-12, which adds up to ~380-810ms per command! For that reason, avoid using the default "master" branch of this library to send and receive data via SDI-12 while also transmitting other serial data or while looking for other pin change interrupts. We have created [Variants and Branches](https://github.com/EnviroDIY/Arduino-SDI-12#variants-and-branches) of this library (read below) to overcome such limitations when used in combination with alternate variants of SoftwareSerial.

## Compatibility Considerations

This library has been tested with an Arduino Uno (AtMega328p), EnviroDIY Mayfly (AtMega1284p), Adafruit Feather 32u4 (AtMega32u4, identical to Arduino Leonardo), and an Adafruit Feather M0 (SAMD21G18, identical to Arduino Zero).  

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
EnviroDIY_SDI12_PCINT3 is in the Mayfly branch of this repository, and was historically was called "SDI12_mod".  It's been cropped to only control interrupt vector 3,, or PCINT3 (D), which on the AtMega1284p/Mayfly, corresponds to Pins D0-D7.  
It is designed to be compatible with EnviroDIY_SoftwareSerial_PCINT12 library, which which has been modified to only control interupt vectors 1 & 2, which on the AtMega1284p/Mayfly corresponds to pins PCINT1 (B) = Pins D08-D15; PCINT2 (C) = Pins D16-D23.

#### EnviroDIY_SDI12_ExtInts
EnviroDIY_SDI12_ExtInts is the ExtInt branch of this repository. It doesn't control any of the interrupts, but instead relies on an external interrupt management library (like [EnableInterrupt](https://github.com/GreyGnome/EnableInterrupt)) to assign the SDI-12 receive data function to the right pin.  This is the least stable because there's some extra delay because the external library is involved.  It's also the easiest to get working in combination with any other pin change interrupt based library. It can be paired with the  EnviroDIY_SoftwareSerial_PCINT12 or the EnviroDIY_SoftwareSerial_ExtInts libraries.

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
