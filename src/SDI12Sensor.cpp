/**
 * @file SDI12Sensor.cpp
 *
 * @brief This file contains the main class for SDI-12 sensor implementation.
 *
 * The class provides a general solution for sensor command handling defined by
 * SDI-12 Support Group Specification.
 * https://www.sdi-12.org/specification
 *
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <WString.h>

#include "SDI12Sensor.h"

/* Initialize Static Variables */
// Pointer reference to last active SDISensor instance
SDI12Sensor *SDI12Sensor::last_set_active_object_ = nullptr;


/**
 * @brief Construct a default SDI12Sensor::SDI12Sensor object.
 *
 * Sensor address defaults to Zero - '0'
 * Device address can be changed with SDI12Sensor::SetAddress(address).
 *
 * This empty constructor is provided for easier integration with other Arduino libraries.
 *
 * @see SetAddress(const char address)
 * @see SDI12Sensor(const char address)
 */
SDI12Sensor::SDI12Sensor(void) {
    sensor_address_ = SDI12SENSOR_DEFAULT_ADDR;
}


/**
 * @brief Construct a new SDI12Sensor::SDI12Sensor object with the address set.
 *
 * Sensor address defaults to Zero - '0' if address is not alpha numeric.
 * Device address can be changed with SDI12Sensor::SetAddress(address).
 *
 * @param[in] address Single alpha numeric character representation of sensor address
 *
 * @see SetAddress(const char address)
 */
SDI12Sensor::SDI12Sensor(const char address) {
    if (!SetAddress(address)) {
        sensor_address_ = SDI12SENSOR_DEFAULT_ADDR;
    }
}


/**
 * @brief Destroy the SDI12Sensor::SDI12Sensor object
 * @see SDI12Sensor(void)
 * @see SDI12Sensor(const char address)
 */
SDI12Sensor::~SDI12Sensor(void) {
    // Reset active reference if current instance is being destroyed
    if (IsActive()) { ClearLastActive(); }
}


/**
 * @brief Sets the sensor address of the SDI12Sensor object.
 *
 * @param[in] address Single alpha numeric character representation of sensor address
 * @return true Sensor address is alpha numeric and update sucessfull
 * @return false Sensor address was not updated
 *
 * @see Address(void)
 */
bool SDI12Sensor::SetAddress(const char address) {
    if (isalnum(address)) {
        sensor_address_ = address;
        return true;
    }
    return false;
}


/**
 * @brief Gets sensor address.
 *
 * @return char Sensor address
 *
 * @see SetAddress(const char address)
 */
char SDI12Sensor::Address(void) const {
    return sensor_address_;
}


/**
 * @brief Sets the active state of the current SDI12Sensor instance.
 *
 * @param active (optional) defaults: true
 * @return true SDI12 object active status has changed
 * @return false SDI12 object active status was the same and not changed
 */
bool SDI12Sensor::SetActive(const bool active) {
    if (last_set_active_object_ != this && active) {
        last_set_active_object_ = this;
        active_ = true;
        return true;
    } else if (active_ != active) {
        if (last_set_active_object_ == this && !active) {
            ClearLastActive();
        }
        active_ = active;
        return true;
    }
    return false;
}


/**
 * @brief Checks if the current SDI12 object instance is set to active.
 *
 * @see SetActive(bool)
 * @see ClearLastActive(void)
 */
bool SDI12Sensor::IsActive(void) const {
  return active_;
}


/**
 * @brief Get the pointer to last set active mutable SDI12Sensor object.
 *
 * @return SDI12Sensor* Pointer reference to  mutable active object, returns @c nullptr if no device is active
 *
 * @see SetActive(bool)
 * @see ClearLastActive(void)
 */
SDI12Sensor *SDI12Sensor::LastActive(void) {
    return last_set_active_object_;
}


/**
 * @brief Clears the last set active SDI12Sensor object, resets
 * the @p last_set_active_object_ reference to @c nullptr
 *
 * @see IsSetLastActive(void)
 * @see LastActive(void)
 * @see SetActive(bool)
 * @see ClearLastActive(void)
 */
void SDI12Sensor::ClearLastActive(void) {
    last_set_active_object_->active_ = false;
    last_set_active_object_ = nullptr;
}


/**
 * @brief Checks if last active SDI12Sensor object is set.
 *
 * @return true last_set_active_object_ != nullptr
 * @return false last_set_active_object_ == nullptr
 *
 * @see ClearLastActive(void)
 * @see LastActive(void)
 */
bool SDI12Sensor::IsSetLastActive(void) {
    return (last_set_active_object_ != nullptr);
}


// void SDI12Sensor::SendSensorAddress() {
//     char message[5];
//     sprintf(message, "%c\r\n", sensor_address_);
//     sendResponse(String(sensor_address_) + "\r\n");
// }


// void SDI12Sensor::SendSensorID() {
//     char message[36];
//     sprintf(message, "%c%s%s%s%s\r\n", sensor_address_, SDI12SENSOR_SDI12_PROTOCOL, SDI12SENSOR_COMPANY, SDI12SENSOR_MODEL, SDI12SENSOR_VERSION, SDI12SENSOR_OTHER_INFO);
//     sendResponse(message);
//     // sendResponse(String(sensor_address_) + SDI12SENSOR_SDI12_PROTOCOL, SDI12SENSOR_COMPANY, SDI12SENSOR_MODEL, SDI12SENSOR_VERSION, SDI12SENSOR_OTHER_INFO);
// }


/**
 * @brief Counts the length of whole/integral parts of decimal values.
 *
 * Integral part is to the left of the decimal point.
 *
 * @param[in] value Floating/Decimal number
 * @return size_t Number of digits of whole/integral part.
 */
size_t IntegralLength(double value) {
    unsigned int len = 0;
    if (value < 0) { value = -value; }
    int val = (int)value;
    do {
        val /= 10;
        ++len;
    } while (val > 0);
    return len;
}


/**
 * @brief Look up table for Powers of 10, 10^0 to 10^9.
 *
 */
static const double powers_of_10[] = {1,      10,      100,      1000,      10000,
                                      100000, 1000000, 10000000, 100000000, 1000000000};


/**
 * @brief Takes a reverses a null terminated string.
 *
 * @param begin Array in memory where to store null-terminated string
 * @param end Pointer to last non null terminated char in array to be reversed.
 */
static void strreverse(char *begin, char *end) {
    char aux;
    while (end > begin) {
        aux = *end;
        *end-- = *begin;
        *begin++ = aux;
    }
}


/**
 * @brief Converts floats to string, based on stringencoders modp_dtoa2() from
 * https://github.com/client9/stringencoders/blob/master/src/modp_numtoa.h
 *
 * @param[in] value value to be converted
 * @param[out] str Array in memory where to store the resulting null-terminated string.
 *      return: "NaN" if overflow or value is greater than desired fit_len
 * @param[in] prec desired precision [0 - 9], will be affected by fit_len,  default: 0
 * @param[in] fit_len (optional) non negative value for max length of output string
 *      including decimal and sign, does not include null pointer,
 *      default: 0 no length limit
 * @param[in] zero_trail (optional) trailing zeros,  default: false
 * @param[in] pos_sign (optional) show positive sign,  default: true
 * @return size_t  Length of string, returns 0 if NaN
 */
size_t dtoa(double value, char *str, uint8_t prec, uint8_t fit_len, bool zero_trail, bool pos_sign) {
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (!(value == value)) {
        strcpy(str, "NaN");
        return 0;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    bool neg = false;
    if (value < 0) {
        neg   = true;
        value = -value;
    }

    int whole = (int)value;

    if (prec <= 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    // Return NaN if whole digit is greater than max_length including sign
    if (fit_len > 0) {
        size_t len_of_integral = IntegralLength(value);
        if (len_of_integral >= fit_len) {
            strcpy(str, "NaN");
            return 0;
        }

        // Resize precision if greater than would fit otherwise
        // -2 to account for sign and decimal
        if (prec > (fit_len - len_of_integral - 2)) {
            prec = (fit_len - len_of_integral - 2);
            if (!pos_sign && !neg) { prec++; }
        }
    }

    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
       normal printf behavior is to print EVERY whole number digit
       which can be 100s of characters overflowing your buffers == bad
       */
    if (value > thres_max) {
        if (pos_sign) {
            sprintf(str, "%+.*f", prec, neg ? -value : value);
        } else {
            sprintf(str, "%.*f", prec, neg ? -value : value);
        }
        return strlen(str);
    }

    char *wstr = str;
    double   p10_fraction  = (value - whole) * powers_of_10[prec];
    uint32_t int_from_frac = (uint32_t)(p10_fraction);
    double diff_frac = p10_fraction - int_from_frac;
    bool has_decimal = false;
    uint8_t len_of_sigfig = prec;

    if (diff_frac > 0.499) {
        // Round up above 0.49 to account for precision conversion error
        ++int_from_frac;
        /* handle rollover, e.g. case 0.99 with prec 1 is 1.0  */
        if (int_from_frac >= powers_of_10[prec]) {
            int_from_frac = 0;
            ++whole;
        }
    } else if (diff_frac == 0.5) {
        if (prec > 0 && (int_from_frac & 1)) {
            /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        } else if (prec == 0 && (whole & 1)) {
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        }
    }

    if (prec > 0) {
        /* Remove ending zeros */
        if (!zero_trail) {
            while (len_of_sigfig > 0 && ((int_from_frac % 10) == 0)) {
                len_of_sigfig--;
                int_from_frac /= 10;
            }
        }
        if (len_of_sigfig > 0) has_decimal = true;

        while (len_of_sigfig > 0) {
            --len_of_sigfig;
            *wstr++ = (char)(48 + (int_from_frac % 10));
            int_from_frac /= 10;
        }
        if (int_from_frac > 0) { ++whole; }

        /* add decimal */
        if (has_decimal) { *wstr++ = '.'; }
    }

    /* do whole part
     * Take care of sign conversion
     * Number is reversed.
     */
    do {
        *wstr++ = (char)(48 + (whole % 10));
    } while (whole /= 10);
    if (neg) { *wstr++ = '-'; } else if (pos_sign) { *wstr++ = '+'; }
    *wstr = '\0';
    strreverse(str, wstr - 1);
    return (size_t)(wstr - str);
}

/**
 * @overload
 * Support for wide string
 */
size_t dtoa(double value, String &str, uint8_t prec, uint8_t fit_len, bool zero_trail, bool pos_sign) {
    /* Hacky test for NaN
     * under -fast-math this won't work, but then you also won't
     * have correct nan values anyways.  The alternative is
     * to link with libmath (bad) or hack IEEE double bits (bad)
     */
    if (!(value == value)) {
        str = "NaN";
        return 0;
    }

    /* we'll work in positive values and deal with the
       negative sign issue later */
    bool neg = false;
    if (value < 0) {
        neg   = true;
        value = -value;
    }

    int whole = (int)value;

    if (prec <= 0) {
        prec = 0;
    } else if (prec > 9) {
        /* precision of >= 10 can lead to overflow errors */
        prec = 9;
    }

    // Return NaN if whole digit is greater than max_length including sign
    if (fit_len > 0) {
        size_t len_of_integral = IntegralLength(value);
        if (len_of_integral >= fit_len) {
            str = "NaN";
            return 0;
        }

        // Resize precision if greater than would fit otherwise
        // -2 to account for sign and decimal
        if (prec > (fit_len - len_of_integral - 2)) {
            prec = (fit_len - len_of_integral - 2);
            if (!pos_sign && !neg) { prec++; }
        }
    }

    /* if input is larger than thres_max, revert to exponential */
    const double thres_max = (double)(0x7FFFFFFF);
    /* for very large numbers switch back to native sprintf for exponentials.
       anyone want to write code to replace this? */
    /*
       normal printf behavior is to print EVERY whole number digit
       which can be 100s of characters overflowing your buffers == bad
       */
    if (value > thres_max) {
        if (neg) {
            str = String(-value, prec);
        } else if (pos_sign) {
            str = "+";
            str += String(value, prec);
        } else {
            str = String(value, prec);
        }
        return str.length();
    }

    double   p10_fraction  = (value - whole) * powers_of_10[prec];
    uint32_t int_from_frac = (uint32_t)(p10_fraction);
    double diff_frac = p10_fraction - int_from_frac;
    uint8_t len_of_sigfig = prec;

    if (diff_frac > 0.499) {
        // Round up above 0.49 to account for precision conversion error
        ++int_from_frac;
        /* handle rollover, e.g. case 0.99 with prec 1 is 1.0  */
        if (int_from_frac >= powers_of_10[prec]) {
            int_from_frac = 0;
            ++whole;
        }
    } else if (diff_frac == 0.5) {
        if (prec > 0 && (int_from_frac & 1)) {
            /* if halfway, round up if odd, OR
           if last digit is 0.  That last part is strange */
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        } else if (prec == 0 && (whole & 1)) {
            ++int_from_frac;
            if (int_from_frac >= powers_of_10[prec]) {
                int_from_frac = 0;
                ++whole;
            }
        }
    }

    if (prec > 0) {
        /* Remove ending zeros */
        if (!zero_trail) {
            while (len_of_sigfig > 0 && ((int_from_frac % 10) == 0)) {
                len_of_sigfig--;
                int_from_frac /= 10;
            }
        }
    }

    /* start building string */
    if (neg) {
        str = String(-value, len_of_sigfig);
    } else if (pos_sign) {
        str = "+";
        str += String(value, len_of_sigfig);
    } else {
        str = String(value, len_of_sigfig);
    }
    return (size_t)(str.length());
}


// void FormatOutputSDI(float *measurementValues, String *dValues, unsigned int maxChar) {
//     /* Ingests an array of floats and produces Strings in SDI-12 output format */

//     uint8_t lenValues = sizeof(*measurementValues) / sizeof(char *);
//     uint8_t lenDValues = sizeof(*dValues) / sizeof(char *);
//     dValues[0] = "";
//     int j = 0;

//     // upper limit on i should be number of elements in measurementValues
//     for (int i = 0; i < lenValues; i++) {
//         // Read float value "i" as a String with 6 decimal digits
//         // (NOTE: SDI-12 specifies max of 7 digits per value; we can only use 6
//         //  decimal place precision if integer part is one digit)
//         String valStr = String(measurementValues[i], SDI12_DIGITS_MAX - 1);

//         // Explictly add implied + sign if non-negative
//         if (valStr.charAt(0) != '-') {
//             valStr = '+' + valStr;
//         }

//         // Append dValues[j] if it will not exceed 35 (aM!) or 75 (aC!) characters
//         if (dValues[j].length() + valStr.length() < maxChar) {
//             dValues[j] += valStr;
//         }
//         // Start a new dValues "line" if appending would exceed 35/75 characters
//         else {
//             dValues[++j] = valStr;
//         }
//     }

//     // Fill rest of dValues with blank strings
//     while (j < lenDValues) {
//         dValues[++j] = "";
//     }
// }
