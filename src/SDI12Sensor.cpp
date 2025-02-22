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

/* Internal ERROR MACRO */
#define ERROR_INVALID_FORMAT -2
#define ERROR_IS_METAGROUP_SEP -3

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


/**
 * @brief Manually set a flag reference to remember if CRC was requested. Not
 * required if already using SDI12Sensor::ParseCommand(received, ack_address).
 *
 * @param crc_request (true/false) If a crc was requested
 *
 * @see ParseCommand(char*, char)
 * @see CrcRequested(void)
 */
void SDI12Sensor::SetCrcRequest(const bool crc_request) {
    crc_requested_ = crc_request;
}


/**
 * @brief Gets the reference flag if a CRC was requested from the sensor.
 *
 * @see ParseCommand(char*, char)
 * @see SetCrcRequest(bool)
 */
bool SDI12Sensor::CrcRequested(void) const {
    return crc_requested_;
}


/**
 * @brief Translates the incoming command to the enumerated type SDI12SensorCommand_e,
 * does not apply any rules.
 *
 * @param received Incoming string command
 * @return SDI12SensorCommand_e Enumeration of received command
 *
 * @see ParseCommand(char*, char)
 */
SDI12SensorCommand_e SDI12Sensor::ReadCommand(const char* received) {
    uint8_t len = strlen(received);
    char first_char = kUnknown;
    char second_char = kUnknown;
    char third_char = kUnknown;

    if (len >= 3) {
        first_char = received[0];
        second_char = received[1];
        third_char = received[2];
    } else if (len >= 2) {
        first_char = received[0];
        second_char = received[1];
    } else if (len >= 1) {
        first_char = received[0];
    } else {
        // Not enough info to bother proceeding
        return kUnknown;
    }

    switch (first_char) {
        case '?': // Query Address, v1.2+
            if (second_char == '!' || second_char == '\0') { return kAddressQuery; }
            break;
        case 'I': // Identification, v1.0+
            return kIdentification;
            break;
        case 'A': // Change address v1.2+
            if (isgraph(second_char) &&
                    (third_char == '!' || third_char == '\0')) {
                return kAddressChange;
            }
            break;
        case 'M': // Blocking Measurement v1.0+
            return kMeasurement;
            break;
        case 'C': // Concurrent Measurement v1.2+
            return kConcurrentMeasurement;
            break;
        case 'D': // Data Request
            if (second_char == 'B') {
                return kByteDataRequest; // v1.4+
            } else {
                return kDataRequest; // v1.0+
            }
            break;
        case 'R': // Continous Measurement v1.2+
            return kContinuousMeasurement;
            break;
        case 'V': // Verification v1.0+
            return kVerification;
            break;
        case 'H': // High Volume Measurement v1.4+
            if (second_char == 'A') {
                return kHighVolumeASCII;
            } else if (second_char == 'B') {
                return kHighVolumeByte;
            }
            break;
        case 'X':
            if (second_char && second_char != '!') { return kExtended; }
            break;
        default: // For debugging purposes
            return kUnknown;
            break;
    }
    return kUnknown;
}


/**
 * @brief Private support function to test supported secondary command for
 * Identification command. aIx...
 *
 * @param[in] cmd Secondary command
 * @return true - Valid Identification second command.
 * @return false - Invalid Identification second command.
 */
bool IdentificationSecondaryCommand(SDI12SensorCommand_e cmd) {
    switch (cmd) {
        case kMeasurement: // Fall Through
        case kVerification: // Fall through
        case kConcurrentMeasurement: // Fall Through
        case kContinuousMeasurement: // Fall Through
        case kHighVolumeASCII: // Fall Through
        case kHighVolumeByte: // Fall Through
        case kExtended:
            return true;
        default:
            return false;
    }
}


/**
 * @brief Decipher received commands and apply rules to determine if the command
 * is in appropriate format.
 *
 * @param[in] received Incoming string reference which contains the command
 * @param[in] ack_address (Optional) Sensor address to determine if received command
 * is an acknowledge command for sensor instance, use @p '\0' if not required to
 * match any particular address
 * @param[out] _endptr Pointer to reference char after command parsing, usefull
 * for extended command. aX123, pointer will point to '1'
 * @return const SDI12CommandSet_s - Received commands structure separated into their
 * primary, and first param, and secondary and secondary parameters.
 */
const SDI12CommandSet_s SDI12Sensor::ParseCommand(const char* received, const char ack_address, char **_endptr) {
    SDI12CommandSet_s parsed_command; // Parsed command reference object
    SDI12SensorCommand_e cmd1 = kUnknown; // Reference to primary command
    SDI12SensorCommand_e cmd2 = kUnknown; // Reference to secondary command
    int param1 = 0; // Reference to primary command parameter
    int param2 = 0; // Reference to secondary command parameter
    uint8_t flags = 0x00; // Reference flag for command parsing info
    char* param_end = nullptr; // Pointer reference for end of parameter

    if (_endptr) { *_endptr = (char*)&received[0]; }
    if (!received) {return parsed_command;} // Process nothing

    // Get first command instruction set
    if (received[1] == '!' || received[1] == '\0') {
        // command string length is <= 2
        if (received[0] == ack_address || isalnum(received[0])) {
            parsed_command.address = *received;
            parsed_command.primary = kAcknowledge; // Acknowledge, v1.0+
        } else {
            // Expect to capture QueryAddress here only
            parsed_command.primary = ReadCommand(received);
        }
    } else if ((received[0] == ack_address || isalnum(received[0])) && received[1] != '?') {
        // Store address, move pointer along and read instruction
        parsed_command.address = *received++;
        cmd1 = ReadCommand(received);
    }

    /* Escape at this point as cmd1 is either UNK for string of all lengths,
    or cmd1 is ACK or QueryAddress for string length of <=2 */
    if (cmd1 == kUnknown) { return parsed_command; }

    // Moving pointer to end of first instruction
    received++;
    switch (cmd1) {
        case kExtended:
            if (_endptr) { *_endptr = (char*)&received[0]; }
            while (isalpha(*received)) {
                received++;
            }
            break;
        case kHighVolumeASCII: // Fall through
        case kHighVolumeByte: // Fall through
        case kByteDataRequest:
            received++;
            // Fall Through
        default:
            // Pointer at char after first instruction
            break;
    }

    /* Get second instruction set if exist */
    if (cmd1 == kIdentification && received) {
        // v1.4 Identify metadata command support
        cmd2 = ReadCommand(received);
        if (cmd2 != kUnknown) {
            received++;
            if (cmd2 == kHighVolumeASCII || cmd2 == kHighVolumeByte) {
                received++;
            }
            // Pointer at char after second instruction
        }
    }

    /* Parse CRC request for the follwing instruction group, v1.3+ */
    if (*received == 'C') {
        switch (cmd1) {
            case kIdentification:
                // v1.4 Identify metada command support
                if (cmd2 != kUnknown && cmd2 == kVerification) {
                    break; // CRC not required for aI and aIV
                }
                // Fall Through
            case kMeasurement: // Fall Through
            case kConcurrentMeasurement: // Fall Through
            case kContinuousMeasurement: // Fall Through
                parsed_command.crc_requested = true;
                received++; // Pointer at char after CRC request 'C'
                break;
            default:
                break;
        }
    } else if (cmd1 == kHighVolumeASCII || cmd1 == kHighVolumeByte) {
        // Command automatically includes CRC request
        parsed_command.crc_requested = true;
    }

    /* Parse First parameter, option/group number for the following instruction sets */
    switch (cmd1) {
        case (kAddressChange):
            // Store the new desired address as param and move pointer along
            param1 = (char)*received++;
            break;
        case kIdentification:
            if (!IdentificationSecondaryCommand(cmd2)) { break; }
            // Fall Through
        case kMeasurement: // Fall Through
        case kDataRequest: // Fall Through
        case kConcurrentMeasurement: // Fall Through
        case kContinuousMeasurement: // Fall Through
        case kByteDataRequest:
        case kExtended:
            param1 = strtol(received, &param_end, 10);
            if (received != param_end) {
                SET_BITS(flags, CMD_PARAM1_FLAG);
                if (*received == '+' || *received == '-') {
                    SET_BITS(flags, CMD_PARAM_SIGN_FLAG);
                    received++;
                }
                // Scan remaining char up to terminator or kIdentification metagroup separator
                if ((*param_end == '!' || *param_end == '_' || *param_end == '\0') &&
                        !(*received == '0' && param1 != 0)) {
                    // Nothing wrong, up to termination is numeric, no lead zero.
                    received = param_end; // Pointer at char after Param1
                } else {
                    for (size_t i = 0; i < strlen(received); i++) {
                        if (!isdigit(received[i]) || (*received == '0' && param1 != 0)) {
                            // Leading zeros or trailing non numeric characters
                            SET_BITS(flags, CMD_PARAM_ERR_FLAG);
                            break;
                        }
                    }
                }
            }
            break;
        default:
            break;
    }


    /* Parse identity metadata parameter group/option, v1.4+ */
    if (*received == '_') { SET_BITS(flags, CMD_HAS_META_FLAG); }
    if ((cmd1 == kIdentification || cmd1 == kExtended) &&
            GET_BITS(flags, CMD_HAS_META_FLAG)) {
        param2 = strtol(++received, &param_end, 10);
        if (received != param_end) {
            SET_BITS(flags, CMD_PARAM2_FLAG);
            if (*received == '+' || *received == '-') {
                SET_BITS(flags, CMD_PARAM_SIGN_FLAG);
                received++;
            }
            if (cmd1 == kIdentification) {
                int len_param2 = 0;
                len_param2 = param_end - received;
                if (len_param2 == 3 && (*param_end == '!' || *param_end == '\0')) {
                    // format is correct of aIx_ddd
                    received = param_end; // Move pointer to char after param2
                } else {
                    // Not appropriate format, aIx_ddd
                    SET_BITS(flags, CMD_PARAM_ERR_FLAG);
                }
            } else {
                received = param_end; // Move pointer to char after param2
            }
        }
    }

    // Determine if at end of command, i.e after all digits
    SET_BITS(flags, (*received == '!' || *received == '\0') << CMD_IS_END_BIT);
    if (_endptr && GET_BITS(flags, CMD_IS_END_FLAG)) { *_endptr = (char*)&received[0]; }

    /* Instruction Rule Set */
    parsed_command.flags = flags;
    if (RuleIsContinous(cmd1, param1, flags) ||
            RuleIsMeasurement(cmd1, param1, flags) ||
            RuleIsDataRequest(cmd1, param1, flags) ||
            RuleIsConcurrent(cmd1, param1, flags) ||
            cmd1 == kExtended ||
            RuleIsAddressChange(cmd1, param1, flags) ||
            RuleIsVerify(cmd1, flags) ||
            RuleIsHighVolumeMeasure(cmd1, flags) ||
            RuleIsIdentifyGroup(cmd1, cmd2, param1, param2, flags)) {
        parsed_command.primary = cmd1;
        parsed_command.secondary = cmd2;
        parsed_command.param1 = param1;
        parsed_command.param2 = param2;
    }
    return parsed_command;
}


/**
 * @brief Rule to test if received address change command is correct.
 *
 * aAb!
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsAddressChange(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags) {
    if (cmd == kAddressChange && isalnum(param1) && GET_BITS(flags, CMD_IS_END_FLAG)) {
        return true;
    }
    return false;
}


/**
 * @brief Rule to test if received measurement command is correct.
 *
 * aM! or aM1~9!, aMC! or aMC1~9! with CRC.
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsMeasurement(const SDI12SensorCommand_e cmd, int param1, const uint8_t flags) {
    if (cmd != kMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (!GET_BITS(flags, CMD_PARAM1_FLAG) ||
            (GET_BITS(flags, CMD_PARAM1_FLAG) && param1 >= 1 && param1 <= 9)) {
            return true;
        }
    }
    return false;
}


/**
 * @brief Rule to test if received concurrent measurement is correct.
 *
 * * aC! or aC1~9!, aCC! or aCC1~9! with CRC.
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsConcurrent(const SDI12SensorCommand_e cmd, int param1, const uint8_t flags) {
    if (cmd != kConcurrentMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (!GET_BITS(flags, CMD_PARAM1_FLAG) ||
            (GET_BITS(flags, CMD_PARAM1_FLAG) && param1 >= 1 && param1 <= 9)) {
            return true;
        }
    }
    return false;
}


/**
 * @brief Rule to test if received continuous measurement is correct.
 *
 * aR0~9! or aRC0~9! with CRC
 * .
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsContinous(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags) {
    if (cmd != kContinuousMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (BITS_IS_SET(flags, CMD_PARAM1_FLAG | CMD_IS_END_FLAG) && (param1 >= 0 && param1 <= 9)) {
        return true;
    }
    return false;
}


/**
 * @brief Rule to test if received data request is correct.
 *
 * aD0...999!
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsDataRequest(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags) {
    if ((cmd != kDataRequest && cmd != kByteDataRequest) ||
            GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) {
        return false;
    }

    if (BITS_IS_SET(flags, CMD_PARAM1_FLAG | CMD_IS_END_FLAG) && (param1 >= 0 && param1 <= 999)) {
        return true;
    }
    return false;
}


/**
 * @brief Rule to test if received verify command is correct.
 *
 * aV!
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsVerify(const SDI12SensorCommand_e cmd, const uint8_t flags) {
    if (cmd == kVerification &&
            GET_BITS(flags, CMD_IS_END_FLAG) &&
            !GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) {
        return true;
    }
    return false;
}


/**
 * @brief Rule to test if received high volume measurement command is correct.
 * Belongs to ascii or byte high volume measurement.
 *
 * aHA! or aHB!, CRC is always requested.
 *
 * @param[in] cmd Deciphered command enumerated type
 * @param[in] param1 Parameter associated with command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsHighVolumeMeasure(const SDI12SensorCommand_e cmd, const uint8_t flags) {
    if ((cmd == kHighVolumeASCII || cmd == kHighVolumeByte) &&
            GET_BITS(flags, CMD_IS_END_FLAG) &&
            !GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) {
        return true;
    }
    return false;
}


/**
 * @brief Rule to determine if received identify command is correct
 *
 * aIM!, aIMC! or aIM1...9! or aIMC1...9!
 * aIV!
 * aIC!, aICC! or aIC1...9! or aICC1...9!
 * aIHA! or aIHB!
 *
 * Meta Group:
 * aIM_001...9!, aIMC_001~9!, aIM1~9_001~9! aIMC1~9_001~9!
 * aIV_001! ... aIV_009!
 * //TODO: Fix comment for aIC meta groups when CRC is requested vs normal
 * aIR0[0-9]_001! ... aIR[0-9]_099!
 * aIRC0[0-9]_001! ... aIRC[0-9]_099!
 *
 *
 * @param[in] cmd1 Deciphered primary command enumerated type
 * @param[in] cmd2 Deciphered secondary command enumerated type
 * @param[out] param1 Parameter associated with primary command
 * @param[in] param2 Parameter associated with secondary command
 * @param[in] flags Flags of deciphered string command
 * @return true if valid command.
 * @return false if invalid command.
 */
bool SDI12Sensor::RuleIsIdentifyGroup(const SDI12SensorCommand_e cmd1, const SDI12SensorCommand_e cmd2,
        int param1, const int param2, const uint8_t flags) {
    if (cmd1 != kIdentification || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;

    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (cmd2 == kUnknown) {
            return true;
        } else if (RuleIsMeasurement(cmd2, param1, flags)) {
            if (!GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (!GET_BITS(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 9) {
                return true;
            }
        } else if (RuleIsConcurrent(cmd2, param1, flags)) {
            // TODO: Check meta group parameters for concurrent identification vs when CRC is requested
            if (!GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (!GET_BITS(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 99) {
                return true;
            }
        } else if (RuleIsVerify(cmd2, flags)) {
            if (!GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 9) {
                return true;
            }
        } else if (RuleIsContinous(cmd2, param1, flags)) {
            if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 99) {
                return true;
            }
        } else if (RuleIsHighVolumeMeasure(cmd2, flags)) {
            if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && !GET_BITS(flags, CMD_PARAM1_FLAG) && param2 >= 1 && param2 <= 999) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Set the sensor state based on a command structure set.
 *
 * @param[in] command_set Received command structure set
 * @return true State was different and change was successfull
 * @return false Same state as previous or command set received does not correspond
 * to the sensor instance
 *
 * @see State(void)
 * @see SetState(int8_t)
 */
bool SDI12Sensor::DefineState(const SDI12CommandSet_s command_set) {
    if (command_set.address != sensor_address_) {
        return false;
    }
    SDI12SensorState_e state = (SDI12SensorState_e)state_;
    switch ((SDI12SensorCommand_e)command_set.primary) {
        case kUnknown:
        case kAcknowledge:
        case kAddressChange:
        case kAddressQuery:
        case kByteDataRequest:
        case kDataRequest:
            state = kStateReady;
            break;
        case kIdentification:
            // Set state to normal Ready state for normal Identify or Identify Parameter commands
            // For Identify Measurement commands, set to appropriate measurement state to handle
            switch (command_set.secondary) {
                case kMeasurement:
                    state = kStateMeasurement;
                    break;
                case kConcurrentMeasurement:
                    state = kStateConcurrent;
                    break;
                case kHighVolumeASCII:
                case kHighVolumeByte:
                    state = kStateHighMeasurement;
                    break;
                case kVerification:
                    state = kStateVerify;
                    break;
                default:
                    state = kStateReady;
                    break;
            }
            if (command_set.param2 >= 0) {
                state = kStateReady;
            }
            crc_requested_ = command_set.crc_requested;
            break;
        case kVerification:
            state = kStateVerify;
            break;
        case kMeasurement:
            state = kStateMeasurement;
            crc_requested_ = command_set.crc_requested;
            break;
        case kConcurrentMeasurement:
            state = kStateConcurrent;
            crc_requested_ = command_set.crc_requested;
            break;
        case kHighVolumeASCII:
            // Set to same state as HighVolumeByte request
        case kHighVolumeByte:
            state = kStateHighMeasurement;
            crc_requested_ = command_set.crc_requested;
            break;
        case kContinuousMeasurement:
            state = kStateContinuous;
            crc_requested_ = command_set.crc_requested;
            break;
        case kExtended:
            state = kStateExtended;
            crc_requested_ = command_set.crc_requested;
            break;
    }

    return SetState(state);
}


/**
 * @brief Manually set the state of a sensor instance.
 *
 * @param[in] state State to set the sensor instance to.
 * @return true New state is different to old state
 * @return false New state is the same as the old state
 *
 * @see DefineState(SDI12CommandSet_s)
 * @see State(void)
 */
bool SDI12Sensor::SetState(const int8_t state) {
    if (state != state_) {
        state_ = state;
        return true;
    }
    return false;
}


/**
 * @brief Gets the sensor current state.
 *
 * @return int8_t Integer or SDI12SensorState_e enumerated representation of the
 * sensor state.
 *
 * @see DefineState(SDI12CommandSet_s)
 * @see SetState(int8_t)
 */
int8_t SDI12Sensor::State(void) const {
    return state_;
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
