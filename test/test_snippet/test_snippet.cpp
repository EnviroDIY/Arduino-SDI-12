#ifdef ARDUINO
#include <Arduino.h>
#else
// #include <cstring>
// #include <cstdio>
// #include <ArduinoFake.h>
#endif
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unity.h>

void setUp(void) {
}

void tearDown(void) {
// clean stuff up here
}

/* COMMAND PARSING SNIPPET */

#define BIT(n) (0x1U << (n))
#define SET_BITS(var, mask)     ((var)  |=  (mask))
#define CLEAR_BITS(var, mask)   ((var)  &= ~(mask))
#define FLIP_BITS(var, mask)    ((var)  ^=  (mask))
#define GET_BITS(var, mask)     ((var)  &   (mask))
#define BITS_IS_SET(var, mask)  ((mask) == GET_BITS(var, mask))

#define CMD_IS_END_BIT          0U
#define CMD_PARAM1_BIT          1U
#define CMD_HAS_META_BIT        2U
#define CMD_PARAM2_BIT          3U
#define CMD_PARAM_ERR_BIT       4U
#define CMD_PARAM_SIGN_BIT      5U

#define CMD_IS_END_FLAG         BIT(CMD_IS_END_BIT)
#define CMD_PARAM1_FLAG         BIT(CMD_PARAM1_BIT)
#define CMD_HAS_META_FLAG       BIT(CMD_HAS_META_BIT)
#define CMD_PARAM2_FLAG         BIT(CMD_PARAM2_BIT)
#define CMD_PARAM_ERR_FLAG      BIT(CMD_PARAM_ERR_BIT)
#define CMD_PARAM_SIGN_FLAG     BIT(CMD_PARAM_SIGN_BIT)


#define SDI12SENSOR_ERR_INVALID -1
// #define ERROR_INVALID_FORMAT -2
// #define ERROR_IS_METAGROUP_SEP -3

typedef enum SDI12SensorCommand_e: uint8_t {
    kUnknown               = 0,  // Debug Purposes
    kAcknowledge           = 1,  // a!
    kAddressQuery          = 2,  // ?!
    kIdentification        = 3,  // aI!, aI(M|C|V|HA|HB)(C|"")(0-9)!
    kAddressChange         = 4,  // aAb!
    kMeasurement           = 5,  // aM!, aMC!, aM1~9!, aMC1~9!
    kDataRequest           = 6,  // aD0~9!, aD0~999! for high ascii
    kConcurrentMeasurement = 7,  // aC!, aCC!, aC1~9!, aCC1~9!
    kContinuousMeasurement = 8,  // aR0~9!, aRC0~9!
    kVerification          = 9,  // aV!, aVC!
    kHighVolumeASCII       = 10, // aHA!, aHAC! high volume ascii
    kHighVolumeByte        = 11, // aHB!, aHBC! high volume byte
    kByteDataRequest       = 12, // aDB0~999! for high volume byte
    kExtended              = 13  // aXNNN! Extended, only recommended format
} SDI12SensorCommand_e;

typedef struct SDI12CommandSet_s {
    char address = '\0'; // Address received
    int8_t primary = kUnknown; // Primary command received
    int8_t secondary = kUnknown; // Secondary command received
    int16_t param1 = 0; // Storage of primary command parameter i.e aM1~9 or new address aAb!
    int16_t param2 = 0; // Storage of secondary command parameter, i.e Identify meta group aI_001~999!
    bool crc_requested = false; //
    uint8_t flags = 0x00;
} SDI12CommandSet_s;

/* Forward declaration */
class SDI12Sensor {
  public:
    static const SDI12CommandSet_s ParseCommand(const char* received, const char ack_address = '\0');

  protected:
    static SDI12SensorCommand_e ReadCommand(const char* received);
    static bool RuleIsAddressChange(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags);
    static bool RuleIsMeasurement(const SDI12SensorCommand_e cmd, int *param1, const uint8_t flags);
    static bool RuleIsConcurrent(const SDI12SensorCommand_e cmd, int *param1, const uint8_t flags);
    static bool RuleIsContinous(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags);
    static bool RuleIsDataRequest(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags);
    static bool RuleIsVerify(const SDI12SensorCommand_e cmd, const uint8_t flags);
    static bool RuleIsHighVolumeMeasure(const SDI12SensorCommand_e cmd, const uint8_t flags);
    static bool RuleIsIdentifyGroup(const SDI12SensorCommand_e cmd1, const SDI12SensorCommand_e cmd2,
            int *param1, const int param2, const uint8_t flags);
};

//TODO: ReadCommand() Doxygen
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
            if (second_char == '\0' || second_char == '!') { return kAddressQuery; }
            break;
        case 'I': // Identification, v1.0+
            return kIdentification;
            break;
        case 'A': // Change address v1.2+
            if (isgraph(second_char) &&
                    (third_char == '\0' || third_char == '!')) {
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
            if (second_char != '\0' && second_char != '!') { return kExtended; }
            break;
        default: // For debugging purposes
            return kUnknown;
            break;
    }
    return kUnknown;
}

bool AllowedIdentificationSubCommand(SDI12SensorCommand_e cmd) {
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

const SDI12CommandSet_s SDI12Sensor::ParseCommand(const char* received, const char ack_address) {
    SDI12CommandSet_s parsed_command;
    uint8_t len = strlen(received);
    SDI12SensorCommand_e cmd1 = kUnknown;
    SDI12SensorCommand_e cmd2 = kUnknown;
    int param1 = 0;
    int param2 = 0;
    uint8_t flags = 0x00;

    if (len == 0) { return parsed_command; } // Process nothing

    // Get first command instruction set
    if (received[1] == '\0' || received[1] == '!') {
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
    switch (cmd1) {
        case kExtended:
            while (*received != '\0' && *received != '!' &&
                *received != '-' && *received != '+' && !isdigit(*received)) {
                received++;
            }
            break;
        case kHighVolumeASCII: // Fall through
        case kHighVolumeByte: // Fall through
        case kByteDataRequest:
            received++;
            // Fall Through
        default:
            received++; // Pointer at char after first instruction
            break;
    }

    /* Get second instruction set if exist */
    if (cmd1 == kIdentification && len > 2) {
        // v1.4 Identify metadata command support
        cmd2 = ReadCommand(received);
        if (cmd2 != kUnknown) {
            received++;
            if ((cmd2 == kHighVolumeASCII || cmd2 == kHighVolumeByte)) {
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
            case kHighVolumeASCII: // Fall Through
            case kHighVolumeByte: // Fall Through
                parsed_command.crc_requested = true;
                received++; // Pointer at char after CRC request 'C'
                break;
            default:
                break;
        }
    }

    /* Parse First parameter, option/group number for the following instruction sets */
    char* end_pntr = nullptr;
    switch (cmd1) {
        case (kAddressChange):
            // Store the new desired address as param and move pointer along
            param1 = (char)*received++;
            break;
        case kIdentification:
            if (!AllowedIdentificationSubCommand(cmd2)) { break; }
            // Fall Through
        case kMeasurement: // Fall Through
        case kDataRequest: // Fall Through
        case kConcurrentMeasurement: // Fall Through
        case kContinuousMeasurement: // Fall Through
        case kByteDataRequest:
        case kExtended:
            // param1 = atoi(received);
            param1 = strtol(received, &end_pntr, 10);
            // if (param1 == 0 && *received != '0') {
            if (received != end_pntr) {
                SET_BITS(flags, CMD_PARAM1_FLAG);
                if (*received == '+' || *received == '-') {
                    SET_BITS(flags, CMD_PARAM_SIGN_FLAG);
                    received++;
                }
                // scan remaining char up to terminator or kIdentification metagroup separator
                for (size_t i = 0; i < strlen(received); i++) {
                    if (received[i+1] == '\0' || received[i] == '!' || received[i] == '_') {
                        // Nothing wrong, all char up to termination or separator is numeric
                        // received = &received[i];
                        // if (received[1] == '\0') { received++; }
                        received = end_pntr;
                        break;
                    } else if (!isdigit(received[i]) || (*received == '0' && param1 != 0)) {
                        // Leading zeros or trailing non numeric characters
                        SET_BITS(flags, CMD_PARAM_ERR_FLAG);
                        break;
                    }
                }
            } else if (*end_pntr != '\0' && *end_pntr != '!' && *end_pntr != '_') {
                // False positive 0 case
                SET_BITS(flags, CMD_PARAM_ERR_FLAG);
            }
            break;
        default:
            break;
    }


    /* Parse identity metadata parameter group/option, v1.4+ */
    if (*received == '_') { SET_BITS(flags, CMD_HAS_META_FLAG); }
    if ((cmd1 == kIdentification || cmd1 == kExtended) &&
            GET_BITS(flags, CMD_HAS_META_FLAG)) {
        param2 = strtol(++received, &end_pntr, 10);
        if (received != end_pntr) {
            SET_BITS(flags, CMD_PARAM2_FLAG);
            if (*received == '+' || *received == '-') {
                SET_BITS(flags, CMD_PARAM_SIGN_FLAG);
                received++;
            }
            for (size_t i = 0; i < strlen(received); i++) {
                if (!isdigit(received[i]) ||
                        (cmd1 == kIdentification && i < 2 &&
                        ((received[i+1] == '\0' || received[1] == '!'))) ) {
                    // Not appropriate format, ddd
                    SET_BITS(flags, CMD_PARAM_ERR_FLAG);
                    break;
                } else if (received[i+1] == '\0' || received[i] == '!') {
                    // Nothing wrong, all char up to termination or separator is numeric
                    received = end_pntr;
                    break;
                }
            }
        } else if (*end_pntr != '\0' && *end_pntr != '!') {
            // False positive 0 case
            SET_BITS(flags, CMD_PARAM_ERR_FLAG);
        }
    }
    // char *meta_group = strchr(received, '_');
    // if (cmd1 == kIdentification && meta_group != NULL) {
    //     received = meta_group + 1; // advance to next char after '_'
    //     for (int i = 0; i < 3; i++) {
    //         if (!isdigit(received[i]) ||
    //                 (i == 2 && (received[i+1] != '\0' && received[i+1] != '!'))) {
    //             param2 = ERROR_INVALID_FORMAT; // Not appropriate format, ddd
    //             break;
    //         }
    //     }
    //     if (param2 != ERROR_INVALID_FORMAT) {
    //         // Should be start of number
    //         param2 = atoi(received);
    //         SET_BITS(flags, CMD_PARAM2_FLAG);
    //     }
    //     // Already expect Identify Meta command, fail if meta param not valid
    //     if (param2 < 0) {return parsed_command; }
    // }


    // Determine if at end of command, i.e after all digits
    // bool is_end = (*received == '\0' || *received == '!');
    SET_BITS(flags, (*received == '\0' || *received == '!') << CMD_IS_END_BIT);
    // if (cmd1 == kIdentification) {
    //     parsed_command.primary = cmd1;
    //     parsed_command.secondary = cmd2;
    //     parsed_command.param1 = param1;
    //     parsed_command.param2 = param2;
    //     return parsed_command;
    // }

    /* Instruction Rule Set */
    // if ((meta_group == NULL &&
    //         (RuleIsMeasurement(cmd1, &param1, is_end) ||
    //         RuleIsDataRequest(cmd1, param1) ||
    //         RuleIsConcurrent(cmd1, &param1, is_end) ||
    //         RuleIsContinous(cmd1, param1) ||
    //         RuleIsHighVolumeMeasure(cmd1, param1, is_end) ||
    //         RuleIsVerify(cmd1, param1, is_end) ||
    //         RuleIsAddressChange(cmd1, param1, is_end)) ) ||
    //         RuleIsIdentifyGroup(cmd1, cmd2, &param1, param2, is_end)) {
    //     parsed_command.primary = cmd1;
    //     parsed_command.secondary = cmd2;
    //     parsed_command.param1 = param1;
    //     parsed_command.param2 = param2;
    //     return parsed_command;
    // }
    parsed_command.flags = flags;
    if (cmd1 == kExtended ||
        RuleIsMeasurement(cmd1, &param1, flags) ||
        RuleIsAddressChange(cmd1, param1, flags) ||
        RuleIsDataRequest(cmd1, param1, flags) ||
        RuleIsConcurrent(cmd1, &param1, flags) ||
        RuleIsContinous(cmd1, param1, flags) ||
        RuleIsVerify(cmd1, flags) ||
        RuleIsHighVolumeMeasure(cmd1, flags) ||
        RuleIsIdentifyGroup(cmd1, cmd2, &param1, param2, flags)
        ) {
        parsed_command.primary = cmd1;
        parsed_command.secondary = cmd2;
        parsed_command.param1 = param1;
        parsed_command.param2 = param2;
        // return parsed_command;
    }

    // Failed all rule check
    // parsed_command.crc_requested = false;
    return parsed_command;
}

bool SDI12Sensor::RuleIsAddressChange(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags) {
    if (cmd == kAddressChange && isalnum(param1) && GET_BITS(flags, CMD_IS_END_FLAG)) {
        return true;
    }
    return false;
}


bool SDI12Sensor::RuleIsMeasurement(const SDI12SensorCommand_e cmd, int *param1, const uint8_t flags) {
    if (cmd != kMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (!GET_BITS(flags, CMD_PARAM1_FLAG) ||
            (GET_BITS(flags, CMD_PARAM1_FLAG) && *param1 >= 1 && *param1 <= 9)) {
            return true;
        }
    }
    return false;
}


bool SDI12Sensor::RuleIsConcurrent(const SDI12SensorCommand_e cmd, int *param1, const uint8_t flags) {
    if (cmd != kConcurrentMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (!GET_BITS(flags, CMD_PARAM1_FLAG) ||
            (GET_BITS(flags, CMD_PARAM1_FLAG) && *param1 >= 1 && *param1 <= 9)) {
            return true;
        }
    }
    return false;
}


bool SDI12Sensor::RuleIsContinous(const SDI12SensorCommand_e cmd, const int param1, const uint8_t flags) {
    if (cmd != kContinuousMeasurement || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;
    if (BITS_IS_SET(flags, CMD_PARAM1_FLAG | CMD_IS_END_FLAG) && (param1 >= 0 && param1 <= 9)) {
        return true;
    }
    return false;
}


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


bool SDI12Sensor::RuleIsVerify(const SDI12SensorCommand_e cmd, const uint8_t flags) {
    if (cmd == kVerification &&
            GET_BITS(flags, CMD_IS_END_FLAG) &&
            !GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM_ERR_FLAG)) {
        return true;
    }
    // if (cmd == kVerification && (is_end || param1 == ERROR_IS_METAGROUP_SEP)) {
    //     return true;
    // }
    return false;
}


bool SDI12Sensor::RuleIsHighVolumeMeasure(const SDI12SensorCommand_e cmd, const uint8_t flags) {
    if ((cmd == kHighVolumeASCII || cmd == kHighVolumeByte) &&
            GET_BITS(flags, CMD_IS_END_FLAG) &&
            !GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM_ERR_FLAG)) {
        return true;
    }
    return false;
}


bool SDI12Sensor::RuleIsIdentifyGroup(const SDI12SensorCommand_e cmd1, const SDI12SensorCommand_e cmd2,
        int *param1, const int param2, const uint8_t flags) {
    if (cmd1 != kIdentification || GET_BITS(flags, CMD_PARAM_ERR_FLAG | CMD_PARAM_SIGN_FLAG)) return false;

    if (GET_BITS(flags, CMD_IS_END_FLAG)) {
        if (cmd2 == kUnknown) {
            return true;
        } else if (RuleIsMeasurement(cmd2, param1, flags) ||
                RuleIsConcurrent(cmd2, param1, flags)) {
            if (!GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (!GET_BITS(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 9) {
                return true;
            }
        } else if (RuleIsVerify(cmd2, flags)) {
            if (!GET_BITS(flags, CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG)) {
                return true;
            } else if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 9) {
                return true;
            }
        } else if (RuleIsContinous(cmd2, *param1, flags)) {
            if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && param2 >= 1 && param2 <= 99) {
                return true;
            }
        } else if (RuleIsHighVolumeMeasure(cmd2, flags)) {
            if (BITS_IS_SET(flags, CMD_PARAM2_FLAG | CMD_HAS_META_FLAG) && !GET_BITS(flags, CMD_PARAM1_FLAG) && param2 >= 1 && param2 <= 999) {
                return true;
            }
        }
    }
//     if (cmd1 == kIdentification) {
//         // return true;
//         if (
//             (cmd2 == kUnknown && is_end) ||
//             RuleIsMeasurement(cmd2, param1, is_end)
//             ) {
//             return true;
//         }
//     }
//     // if (cmd1 == kIdentification &&
//     //         (((param2 == SDI12SENSOR_ERR_INVALID || (param2 >= 1 && param2 <= 999)) &&
//     //         ((is_end && cmd2 == kUnknown) ||
//     //         RuleIsMeasurement(cmd2, param1, is_end) ||
//     //         RuleIsConcurrent(cmd2, param1, is_end) ||
//     //         RuleIsVerify(cmd2, *param1, is_end) ||
//     //         RuleIsHighVolumeMeasure(cmd2, *param1, is_end))) ||
//     //         (RuleIsContinous(cmd2, *param1) && (param2 >= 1 && param2 <= 999)))
//     //         ) {
//     //     return true;
//     // }
    return false;
}





// #define _DEBUG
char command_str[] = "aIR0_0010";

void test_parse_command_primary_command() {
    SDI12CommandSet_s cmd = SDI12Sensor::ParseCommand(command_str, 'a');
    char response[20];

    switch (cmd.primary) {
        case kAcknowledge: // Accknowledge
        case kAddressQuery: // Query Address
        case kIdentification: // Identification
        case kAddressChange: // Change address
        case kMeasurement: // Blocking Measurement
        case kConcurrentMeasurement: // Concurrent Measurement
        case kDataRequest: // Data Request ASCII
        case kContinuousMeasurement: // Continous Measurement
        case kVerification: // Verification
        case kHighVolumeASCII: // High Volume Ascii
        case kHighVolumeByte: // High Volume Byte
        case kByteDataRequest: // Data Request Byte
            sprintf(response, "MatchOn = %i", cmd.primary);
            TEST_PASS_MESSAGE(response);
            break;
        default:
            sprintf(response, "Received = %i", cmd.primary);
            TEST_FAIL_MESSAGE(response);
            break;
    }
}

void test_parse_command_secondary_command() {
    SDI12CommandSet_s cmd = SDI12Sensor::ParseCommand(command_str,  'a');
    char response[20];

    switch (cmd.secondary) {
        case kAcknowledge: // Accknowledge
        case kAddressQuery: // Query Address
        case kIdentification: // Identification
        case kAddressChange: // Change address
        case kMeasurement: // Blocking Measurement
        case kConcurrentMeasurement: // Concurrent Measurement
        case kDataRequest: // Data Request
        case kContinuousMeasurement: // Continous Measurement
        case kVerification: // Verification
        case kHighVolumeASCII: // High Volume Ascii
        case kHighVolumeByte: // High Volume Byte
        case kByteDataRequest: // Data Request
            sprintf(response, "MatchOn = %i", cmd.secondary);
            TEST_PASS_MESSAGE(response);
            break;
        default:
            sprintf(response, "Received = %i", cmd.secondary);
            TEST_FAIL_MESSAGE(response);
            break;
    }
}

void test_parse_command_crc_request() {
    SDI12CommandSet_s cmd = SDI12Sensor::ParseCommand(command_str, 'a');
    TEST_ASSERT_TRUE(cmd.crc_requested);
}

void test_parse_command_option_group() {
    SDI12CommandSet_s cmd = SDI12Sensor::ParseCommand(command_str, 'a');
    char response[20];
    for (int i = 0; i <= 999; i++) {
        if (i == cmd.param1) {
            sprintf(response, "MatchOn = %i", cmd.param1);
            TEST_PASS_MESSAGE(response);
        }
    }
    // TEST_FAIL();
    sprintf(response, "Received = %i", cmd.param1);
    TEST_FAIL_MESSAGE(response);
}

void test_parse_command_param_group() {
    SDI12CommandSet_s cmd = SDI12Sensor::ParseCommand(command_str, 'a');
    char response[20];
    for (int i = 0; i <= 999; i++) {
        if (i == cmd.param2) {
            sprintf(response, "MatchOn = %i", cmd.param2);
            TEST_PASS_MESSAGE(response);
        }
    }
    // TEST_FAIL();
    sprintf(response, "Received = %i", cmd.param2);
    TEST_FAIL_MESSAGE(response);
}

void test_parse_ack() {
    char addresses[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    // const char *ptr = addresses;
    char msg[3] = "";
    SDI12CommandSet_s cmd;

    // Weird case test where we assume '?' is a valid address, expect kAcknowledge command
    cmd = SDI12Sensor::ParseCommand("?", '?');
    TEST_ASSERT_EQUAL_MESSAGE(kAcknowledge, cmd.primary,
            "yes!; Address is '?' should be matched as a Ack command");

    // NO need to test leading and traling char as it would not be an acknowledge command

    // Correct format, terminated and unterminated
    for (size_t i = 0; i < strlen(addresses); i++) {
        #ifdef ARDUINO
            msg[0] = addresses[i];
            msg[1] = '\0';
        #else
            strncpy(msg, &addresses[i], 1);
        #endif
        cmd = SDI12Sensor::ParseCommand(msg, addresses[i]);
        TEST_ASSERT_EQUAL(addresses[i], cmd.address);
        TEST_ASSERT_EQUAL_MESSAGE(kAcknowledge, cmd.primary, msg);

        #ifdef ARDUINO
            msg[2] = '!';
            msg[3] = '\0';
        #else
            strcat(msg, "!");
        #endif
        cmd = SDI12Sensor::ParseCommand(msg, addresses[i]);
        TEST_ASSERT_EQUAL(addresses[i], cmd.address);
        TEST_ASSERT_EQUAL_MESSAGE(kAcknowledge, cmd.primary, msg);
    }

}

void test_parse_query_addr() {
    char msg[6] = "?";
    SDI12CommandSet_s cmd;

    cmd = SDI12Sensor::ParseCommand(msg);
    TEST_ASSERT_EQUAL(kAddressQuery, cmd.primary);
    // Test query address does not return address
    TEST_ASSERT_EQUAL('\0', cmd.address);

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL(kAddressQuery, cmd.primary);

    /* primary unterminated test */
    // Weird case test where we assume '?' is a valid address, expect kAcknowledge command
    cmd = SDI12Sensor::ParseCommand(msg, '?');
    TEST_ASSERT_EQUAL_MESSAGE(kAcknowledge, cmd.primary,
            "no!; Address is '?' should be matched as a ACK command");

    // extra character after '?', expect kUnknown
    strcat(msg, "a");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "no !; extra char after ?; should be kUnknown");


    /* primary terminated test */
    // Normal terminated
    strcpy(msg, "?!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL(kAddressQuery, cmd.primary);


    /* leading char tests */
    // lead char, no terminator, expect kUnknown
    strcpy(msg, "a?");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "no !; extra char before ?, should be kUnknown");

    // lead char, terminator, expect kUnknown
    strcpy(msg, "a?!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "yes !; extra char before ?, should be kUnknown");

    // lead char, no terminator, Weird case test where we assume '?' is a valid address, expect kUnknown command
    strcpy(msg, "a?");
    cmd = SDI12Sensor::ParseCommand(msg, '?');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary,
            "no!; Address is '?' lead char");

    // lead char, terminator, Weird case test where we assume '?' is a valid address, expect kUnknown command
    strcpy(msg, "a?!");
    cmd = SDI12Sensor::ParseCommand(msg, '?');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary,
            "yes!; Address is '?' lead char");
}

void test_parse_measurement_addr_param_matches() {
    // Unterminated test
    char msg[6] = "aM";
    SDI12CommandSet_s cmd;
    char response[50];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_BITS_LOW(CMD_PARAM1_FLAG|CMD_PARAM2_FLAG, cmd.flags);
    TEST_ASSERT_BITS_HIGH(CMD_IS_END_FLAG, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aM!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_BITS_LOW(CMD_PARAM1_FLAG|CMD_PARAM2_FLAG, cmd.flags);
    TEST_ASSERT_BITS_HIGH(CMD_IS_END_FLAG, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aM%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        // TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aM%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        if (i < 1 || i > 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary,
                    "!: aMx out of bounds, expect kUnknown");
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1,
                    "!: aMx out of bounds, param1 should be -1");
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary,
                    "!: aMx In bounds, should be measurement cmd");
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1,
                    "!: aMx In bounds, param1 should equal");
        }
    }

    //trailing char, terminated
    strcpy(msg, "aMa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //lead char, terminated
    strcpy(msg, "aaM!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "!: leading char same as addr, expect unknown command");
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1,
                    "!: aaMx not numeric, param1 should be -1");

    //param leading zero, not terminated
    strcpy(msg, "aM01!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "!: aM01 not numeric, param1 should be -1");
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "no!: param leading 0, expect unknown command");

    //param leading zero, terminated
    strcpy(msg, "aM01!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "!: aM01! not numeric, param1 should be -1");
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "!: param leading 0, expect unknown command");
}

void test_parse_identify_matches() {
    char response[100];
    // Unterminated test
    char msg[6] = "aI";
    SDI12CommandSet_s cmd;

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aI!");
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing char, not terminated
    strcpy(msg, "aIa");
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);

    //trailing char, terminated
    strcpy(msg, "aIa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "aIa!: trail char same as addr, expect unknown command");

    //lead char, not terminated
    strcpy(msg, "aaI");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "aaI: lead char same as addr, expect unknown command");

    //lead char, terminated
    strcpy(msg, "aaI!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, "aaI!: lead char same as addr, expect unknown command");
}

void test_parse_address_change_addr_param_matches() {
    // Unterminated test
    char msg[6] = "aAb";
    SDI12CommandSet_s cmd;

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kAddressChange, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE('b', cmd.param1, "aAb, expect b");

    //Terminated command
    strcpy(msg, "aAb!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kAddressChange, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE('b', cmd.param1, "aAb!, expect b");

    // Inappropriate new address, expect kUnknown
    strcpy(msg, "aA`");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aA`, expect -1");

    // Inappropriate new address, expect kUnknown
    strcpy(msg, "aA`!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aA`!, expect -1");

    //No new address, expect kUnknown
    strcpy(msg, "aA");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aA, expect -1");

    //No new address, expect kUnknown
    strcpy(msg, "aA!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aA!, expect -1");
}

void test_parse_data_request_addr_param_matches(){
    char response[70];
    char msg[10] = "aD";
    SDI12CommandSet_s cmd;

    // Test with no param
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    // test with invalid and valid param
    for (int16_t i = -1; i <= 1003; i++) {
        sprintf(msg, "aD%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 0 && i <= 999) {
            TEST_ASSERT_EQUAL_MESSAGE(kDataRequest, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    // Test with alpha param
    strcpy(msg, "aDa");
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aDa, expect -1");

    //trailing char, valid param
    for (uint16_t i = 0; i <= 999; i++) {
        sprintf(msg, "aD%ia", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    }

    //leading char, valid param
    strcpy(msg, "aaD0");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aaD0, expect -1");
}

void test_parse_verify_addr_param_matches() {
    // Unterminated test
    char msg[6] = "aV";
    SDI12CommandSet_s cmd;
    char response[50];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aV!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing char
    strcpy(msg, "aVa");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aVa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //leading char
    strcpy(msg, "aaV");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aaV!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing digit
    //trailing digit
    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aV%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_PARAM1_FLAG, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    }
}

void test_parse_concurrent_addr_param_matches() {
    // Unterminated test
    char msg[7] = "aC";
    SDI12CommandSet_s cmd;
    char response[50];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_BITS_LOW(CMD_PARAM1_FLAG|CMD_PARAM2_FLAG, cmd.flags);
    TEST_ASSERT_BITS_HIGH(CMD_IS_END_FLAG, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_BITS_LOW(CMD_PARAM1_FLAG|CMD_PARAM2_FLAG, cmd.flags);
    TEST_ASSERT_BITS_HIGH(CMD_IS_END_FLAG, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aC%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aC%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_HAS_META_FLAG, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_BITS_LOW(CMD_PARAM_SIGN_FLAG, cmd.flags);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
        if (i < 0) {
            TEST_ASSERT_BITS_HIGH(CMD_PARAM_SIGN_FLAG, cmd.flags);
        }
    }


    //trailing char, terminated
    strcpy(msg, "aCa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aCa, expect -1");

    //lead char, terminated
    strcpy(msg, "aaC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aaC!, expect -1");
}

void test_parse_continuous_addr_param_matches() {
    // Unterminated test
    char msg[6] = "aR";
    SDI12CommandSet_s cmd;
    char response[50];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aR!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aR%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 0 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aR%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 0 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    //trailing char, terminated
    strcpy(msg, "aRa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aCa, expect -1");

    //lead char, terminated
    strcpy(msg, "aaR!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, "aaC!, expect -1");
}

void test_parse_high_volume_ascii_matches() {
    // Unterminated test
    char msg[6] = "aHA";
    SDI12CommandSet_s cmd;
    char response[70];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aHA!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing char
    strcpy(msg, "aHAa");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHAa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //leading char
    strcpy(msg, "aaHA");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aaHA!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing digit
    strcpy(msg, "aHA0");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHA0!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //CRC
    strcpy(msg, "aHAC");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHAC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
}

void test_parse_high_volume_byte_matches() {
    // Unterminated test
    char msg[6] = "aHB";
    SDI12CommandSet_s cmd;
    char response[70];

    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //Terminated command
    strcpy(msg, "aHB!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing char
    strcpy(msg, "aHBa");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHBa!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //leading char
    strcpy(msg, "aaHB");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aaHB!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, msg);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //trailing digit
    strcpy(msg, "aHB0");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHB0!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    //CRC
    strcpy(msg, "aHBC");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    strcpy(msg, "aHBC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
}

void test_parse_identify_measure_group() {
    char response[100];
    char msg[15];
    SDI12CommandSet_s cmd;

    // Unterminated, Measurement
    strcpy(msg, "aIM");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // Terminated, Measurement
    strcpy(msg, "aIM!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // param 1 set
    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aIM%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }
    for (int8_t i = -5; i <= 10; i++) {
        sprintf(msg, "aIM%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }

    // param2 && metagroup
    strcpy(msg, "aIM_");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIM%i_", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    // Try with incorrect zero
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIM_%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // proper param2 test
    for (int16_t j = -1; j <= 1000; j++) {
        sprintf(msg, "aIM_%03i", j);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        if (j > 0 && j <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }
    for (int8_t i = -5; i <= 12; i++) {
        for (int16_t j = -1; j <= 1000; j++) {
            sprintf(msg, "aIM%i_%03i", i, j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (i >= 1 && i <= 9 && j > 0 && j <= 9) {
                TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
            } else {
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
            }
        }
    }
}

void test_parse_identify_verify_group() {
    char response[100];
    char msg[15];
    SDI12CommandSet_s cmd;

    // Unterminated, Measurement
    strcpy(msg, "aIV");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // Terminated, Measurement
    strcpy(msg, "aIV!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // param 1 set
    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aIV%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    for (int8_t i = -5; i <= 10; i++) {
        sprintf(msg, "aIV%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // param2 && metagroup
    strcpy(msg, "aIV_");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIV%i_", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    // Try with incorrect zero
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIV_%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // proper param2 test
    for (int16_t j = -1; j <= 1000; j++) {
        sprintf(msg, "aIV_%03i", j);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        if (j > 0 && j <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }
}

void test_parse_identify_concurrent_group() {
    char response[100];
    char msg[15];
    SDI12CommandSet_s cmd;

    // Unterminated, Measurement
    strcpy(msg, "aIC");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // Terminated, Measurement
    strcpy(msg, "aIC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // param 1 set
    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aIC%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }
    for (int8_t i = -5; i <= 10; i++) {
        sprintf(msg, "aIC%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }

    // param2 && metagroup
    strcpy(msg, "aIC_");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIC%i_", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    // Try with incorrect zero
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIC_%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // proper param2 test
    for (int16_t j = -1; j <= 1000; j++) {
        sprintf(msg, "aIC_%03i", j);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        if (j > 0 && j <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
    }
    for (int8_t i = -5; i <= 12; i++) {
        for (int16_t j = -1; j <= 1000; j++) {
            sprintf(msg, "aIC%i_%03i", i, j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (i >= 1 && i <= 9 && j > 0 && j <= 9) {
                TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
            } else {
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
            }
        }
    }
}

void test_parse_identify_continuous_group() {
    char response[100];
    char msg[15];
    SDI12CommandSet_s cmd;

    // Unterminated, Measurement
    strcpy(msg, "aIR");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // Terminated, Measurement
    strcpy(msg, "aIR!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    // param 1 set
    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aIR%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    for (int8_t i = -5; i <= 10; i++) {
        sprintf(msg, "aIR%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // param2 && metagroup
    strcpy(msg, "aIR_");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
            cmd.primary, cmd.param1,
            cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIR%i_", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }
    // Try with incorrect zero
    for (int8_t i = -5; i <= 12; i++) {
        sprintf(msg, "aIR_%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
    }

    // proper param2 test
    for (int8_t i = -5; i <= 12; i++) {
        for (int16_t j = -1; j <= 1000; j++) {
            sprintf(msg, "aIR%i_%i", i, j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (i >= 0 && i <= 9 && j > 0 && j <= 99) {
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
            }
        }
    }
    for (int8_t i = -5; i <= 12; i++) {
        for (int16_t j = -1; j <= 1000; j++) {
            sprintf(msg, "aIR%i_%03i", i, j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (i >= 0 && i <= 9 && j > 0 && j <= 99) {
                TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
            } else {
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
            }
        }
    }
}

void test_parse_identify_high_volume_group() {
    char response[100];
    char msg[15];
    SDI12CommandSet_s cmd;
    char cmd_set[3] ="AB";

    for (size_t k = 0; k < strlen(cmd_set); k++) {
        // Unterminated, Measurement
        sprintf(msg, "aIH%c", cmd_set[k]);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

        // Terminated, Measurement
        sprintf(msg, "aIH%c!", cmd_set[k]);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

        // param 1 set
        for (int8_t i = -1; i <= 10; i++) {
            sprintf(msg, "aIH%c", cmd_set[k]);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
        for (int8_t i = -5; i <= 10; i++) {
            sprintf(msg, "aIH%c!", cmd_set[k]);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }

        // param2 && metagroup
        sprintf(msg, "aIH%c_", cmd_set[k]);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        for (int8_t i = -5; i <= 12; i++) {
            sprintf(msg, "aIH%c%i_", cmd_set[k], i);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }
        // Try with incorrect zero
        for (int8_t i = -5; i <= 12; i++) {
            sprintf(msg, "aIH%c_%i", cmd_set[k], i);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        }

        // proper param2 test
        for (int16_t j = -1; j <= 1000; j++) {
            sprintf(msg, "aIH%c_%03i", cmd_set[k], j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (j > 0 && j <= 999) {
                TEST_ASSERT_EQUAL_MESSAGE(kIdentification, cmd.primary, response);
                if (k == 0) {
                    TEST_ASSERT_EQUAL_CHAR_MESSAGE(kHighVolumeASCII, cmd.secondary, response);
                } else {
                    TEST_ASSERT_EQUAL_CHAR_MESSAGE(kHighVolumeByte, cmd.secondary, response);
                }
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(j, cmd.param2, response);
            } else {
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
                TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.secondary, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
                TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
            }
        }
    }
}

void test_parse_CRC_command() {
    // Unterminated test
    char msg[20];
    char response[100];
    SDI12CommandSet_s cmd;

    strcpy(msg, "aM!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aMC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

    strcpy(msg, "aMC1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);
    TEST_ASSERT_EQUAL_MESSAGE(1, cmd.param1, response);

    strcpy(msg, "aV!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kVerification, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aVC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);


    strcpy(msg, "aC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aCC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

    strcpy(msg, "aCC1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kConcurrentMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);
    TEST_ASSERT_EQUAL_MESSAGE(1, cmd.param1, response);

    strcpy(msg, "aR1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aRC1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

    strcpy(msg, "aRC1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kContinuousMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);
    TEST_ASSERT_EQUAL_MESSAGE(1, cmd.param1, response);

    strcpy(msg, "aHA!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aHAC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeASCII, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

    strcpy(msg, "aHB!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);

    strcpy(msg, "aHBC!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kHighVolumeByte, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);
}

void test_parse_command_extended() {
    // Unterminated test
    char msg[20] = "aX";
    char response[100];
    SDI12CommandSet_s cmd;

    // Invalid command
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    //Terminated command
    strcpy(msg, "aX!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);


    strcpy(msg, "aXNNN");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);

    /* Testing first parameter*/
    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aXNNN%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_HAS_META_FLAG | CMD_PARAM2_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
    }

    /* Testing second parameter*/
    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aXNNN0_%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param2, response);
        TEST_ASSERT_BITS_HIGH(CMD_HAS_META_FLAG | CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_IS_END_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
    }

    /* Testing second parameter*/
    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aXNNN0_%ia66", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param2, response);
        TEST_ASSERT_BITS_HIGH(CMD_HAS_META_FLAG | CMD_PARAM1_FLAG | CMD_PARAM2_FLAG | CMD_PARAM_ERR_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_IS_END_FLAG, cmd.flags);
    }

    /* Testing second parameter*/
    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aXXU,U=%i,G=5!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM_ERR_FLAG, cmd.flags);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_IS_END_FLAG | CMD_PARAM2_FLAG | CMD_HAS_META_FLAG, cmd.flags);
    }

}


static void process_tests(void) {
    UNITY_BEGIN();

#ifdef _DEBUG
    RUN_TEST(test_parse_command_primary_command);
    RUN_TEST(test_parse_command_secondary_command);
    RUN_TEST(test_parse_command_crc_request);
    RUN_TEST(test_parse_command_option_group);
    RUN_TEST(test_parse_command_param_group);
#endif

    RUN_TEST(test_parse_ack);
    RUN_TEST(test_parse_query_addr);
    RUN_TEST(test_parse_measurement_addr_param_matches);
    RUN_TEST(test_parse_address_change_addr_param_matches);
    RUN_TEST(test_parse_data_request_addr_param_matches);
    RUN_TEST(test_parse_concurrent_addr_param_matches);
    RUN_TEST(test_parse_continuous_addr_param_matches);
    RUN_TEST(test_parse_verify_addr_param_matches);
    RUN_TEST(test_parse_high_volume_ascii_matches);
    RUN_TEST(test_parse_high_volume_byte_matches);
    RUN_TEST(test_parse_CRC_command);
    RUN_TEST(test_parse_identify_matches);
    RUN_TEST(test_parse_identify_measure_group);
    RUN_TEST(test_parse_identify_verify_group);
    RUN_TEST(test_parse_identify_concurrent_group);
    RUN_TEST(test_parse_identify_continuous_group);
    RUN_TEST(test_parse_identify_high_volume_group);
    RUN_TEST(test_parse_command_extended);
#ifndef ARDUINO
#endif

    UNITY_END();
}

#ifdef ARDUINO
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);
    process_tests();
}

void loop() {
    // Do nothing
}

#else

int main(int argc, char **argv) {
    if (argc || argv) {}
    process_tests();
    return 0;
}

#endif