/**
 * @file SDI12Protocol.h
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 *
 * @brief Shared, role-neutral limits from the SDI-12 protocol.
 */

#ifndef SRC_SDI12PROTOCOL_H_
#define SRC_SDI12PROTOCOL_H_

#include <stdint.h>

namespace sdi12 {
namespace protocol {

/**
 * @brief Normative SDI-12 timing limits in microseconds.
 *
 * These compile-time integral constants have no object storage. The transport,
 * recorder, and sensor layers use the same limits when producing or validating
 * traffic.
 */
struct Timing {
  enum : uint32_t {
    /** General timing tolerance, except where the specification disallows it. */
    kGeneralToleranceMicros = 400UL,
    /** The size of a bit in microseconds;
     * 1200 baud = 1200 bits/second ~ 833.333 µs/bit */
    kBitWidthMicros = 833UL,
    /** Minimum duration of a valid spacing break. */
    kBreakMinMicros = 12000UL,
    /** Maximum time in which a transmitter must relinquish the data line. */
    kLineReleaseMaxMicros = 7500UL,
    /** Minimum marking before the first character of a command or response. */
    kPreCharacterMarkingMinMicros = 8330UL,
    /** Maximum time from command completion to the response start bit. */
    kResponseStartMaxMicros = 15000UL,
    /** Maximum marking between the stop bit of one character and the start bit of the
       next character; no general tolerance applies. */
    kInterCharacterMarkingMaxMicros = 1660UL,
    /** Maximum time for a sensor to wake after detecting a break. */
    kSensorWakeMaxMicros = 100000UL,
    /** Marking duration after which a sensor must return to standby. */
    kSensorStandbyMarkingMinMicros = 100000UL,
    /** Marking duration after which the next recorder command requires a break. */
    kBreakRequiredAfterMarkingMicros = 87000UL
  };
};

/**
 * @brief Normative size limits for SDI-12 data responses.
 *
 * From SDI-12 Protocol v1.4, Section 4.4 SDI-12 Commands and Responses:
 * > The maximum number of characters that can be returned in the [values] part of the
 * > response to a D command is either 35 or 75. If the D command is issued to
 * > retrieve data in response to a concurrent measurement command, or in response to
 * > a high-volume ASCII measurement command, the maximum is 75. The maximum is also
 * > 75 in response to a continuous measurement command. Otherwise, the maximum is 35.
 *
 * The entire time for a typical response to be transmitted by a sensor is
 * approximately 380 milliseconds.  For a high-volume ASCII response, the time
 * stretches out to 780ms, or 810ms if a CRC is included.
 *
 * From SDI-12 Protocol v1.4, Table 11 The send data command (aD0!, aD1! . . . aD9!):
 * > - the maximum number of digits for a data value is 7, even without a decimal
 * > point
 * > - the minimum number of digits for a data value (excluding the decimal point) is
 * > 1
 * > - the maximum number of characters in a data value is 9 (the (polarity sign + 7
 * > digits + the decimal point))
 *
 * - SRGD Note: The polarity symbol (+ or -) acts as a delimeter between the numeric
 * values
 * */
struct ResponseLimits {
  enum : uint16_t {
    /** Maximum characters in one data value, including sign and decimal point. */
    kValueMaxCharacters = 9U,
    /** Maximum characters in the values portion of a standard data response. */
    kStandardValuesMaxCharacters = 35U,
    /** Maximum characters in the values portion of a concurrent or high-volume ASCII
       data response. */
    kExtendedValuesMaxCharacters = 75U,
    /**
     * @brief The maximum number of bytes in a high volume binary data (aDB#!) request
     * response.
     *
     * This does not include the 1 byte address, 2 byte packet size, 1 byte data type,
     * and 2 byte CRC.  A high volume binary data response **does not** end with a
     * CR+LF.
     *
     * Including the extra characters, the maximum transmission time for a high volume
     * binary data response is approximately 10.1 seconds.
     */
    kBinaryDataMaxBytes = 1000U
  };
};

}  // namespace protocol
}  // namespace sdi12

#endif  // SRC_SDI12PROTOCOL_H_
