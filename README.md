Arduino-SDI-12
==============

Arduino library for SDI-12 communications to a wide variety of environmental sensors. This library provides a general software solution, without requiring any additional hardware, to implement the SDI-12 communication protocol between an Arduino-based data logger and SDI12-enabled sensors.

[SDI-12](http://www.sdi-12.org/) is an asynchronous, ASCII, serial communications protocol that was developed for intelligent sensory instruments that typically monitor environmental data. [Advantages of SDI-12](http://en.wikipedia.org/wiki/SDI-12) include the ability to use a single available data channel for many sensors.

This project is part of the [EnviroDIY](http://envirodiy.org/) vision to create an open source hardware and software stack to deliver near real time environmental data from wireless sensor networks, such as the Arduino-compatible [EnviroDIY™ Mayfly Data Logger](http://envirodiy.org/mayfly/).

Note that this library will conflict with SoftwareSerial, the Sodaq PCInt library, and any other library that monopolize all pin change interrupt vectors.  To help in using this in combination with those libraries, there is a version (SDI12_PCINT3) which is identical, except in that it only uses PCINT3 and ignores 0-2.  Get that version from the "Mayfly" branch.

##Getting Started

Read the [Arduino-SDI-12 wiki](https://github.com/StroudCenter/Arduino-SDI-12/wiki).

##Contribute
Open an [issue](https://github.com/EnviroDIY/Arduino-SDI-12/issues) to suggest and discuss potential changes/additions.

For power contributors:

1. Fork it!
2. Create your feature branch: `git checkout -b my-new-feature`
3. Commit your changes: `git commit -am 'Add some feature'`
4. Push to the branch: `git push origin my-new-feature`
5. Submit a pull request :D


##License
The SDI12 library code is released under the GNU Lesser Public License (LGPL 2.1) -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE) file for details.

Example Arduino sketches are released under the BSD 3-Clause License -- See [LICENSE-examples.md](https://github.com/EnviroDIY/Arduino-SDI-12/blob/master/LICENSE.md) file for details.

Documentation is licensed as [Creative Commons Attribution-ShareAlike 4.0](https://creativecommons.org/licenses/by-sa/4.0/) (CC-BY-SA) copyright.

##Credits
[EnviroDIY](http://envirodiy.org/)™ is presented by the Stroud Water Research Center, with contributions from a community of enthusiasts sharing do-it-yourself ideas for environmental science and monitoring.

[Kevin M. Smith](https://github.com/Kevin-M-Smith) is the primary developer of the Arduino-SDI-12 library, with input from [S. Hicks](https://github.com/s-hicks2) and many [other contributors](https://github.com/EnviroDIY/Arduino-SDI-12/graphs/contributors).

This project has benefited from the support from the following funders:

* National Science Foundation, awards [EAR-0724971](http://www.nsf.gov/awardsearch/showAward?AWD_ID=0724971), [EAR-1331856](http://www.nsf.gov/awardsearch/showAward?AWD_ID=1331856), [ACI-1339834](http://www.nsf.gov/awardsearch/showAward?AWD_ID=1339834)
* Stroud Water Research Center endowment
