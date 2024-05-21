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

// #include "../../src/SDI12Sensor.h"
// #include "../../src/SDI12Sensor.cpp"
#include <SDI12Sensor.h>

// #include <../src/SDI12Sensor.cpp>

void setUp(void) {
}

void tearDown(void) {
// clean stuff up here
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
    TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
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
    TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
    TEST_ASSERT_BITS_HIGH(CMD_IS_END_FLAG, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aM%i", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        }
    }

    for (int8_t i = -1; i <= 10; i++) {
        sprintf(msg, "aM%i!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_BITS_HIGH(CMD_PARAM1_FLAG, cmd.flags);
        if (i >= 1 && i <= 9) {
            TEST_ASSERT_EQUAL_MESSAGE(kMeasurement, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(i, cmd.param1, response);
            TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG, cmd.flags);
        } else {
            TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
            TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
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
    strcpy(msg, "aVC");
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
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

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

    // False CRC
    strcpy(msg, "aHAC");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);
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
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

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

    // False CRC
    strcpy(msg, "aHAC");
    cmd = SDI12Sensor::ParseCommand(msg, 'a');
    sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
    TEST_ASSERT_EQUAL_MESSAGE(kUnknown, cmd.primary, response);
    TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
    TEST_ASSERT_EQUAL_MESSAGE(false, cmd.crc_requested, response);
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
    uint8_t min_param1 = 1;
    uint8_t max_param1 = 9;
    uint8_t min_param2 = 1;
    uint8_t max_param2 = 99;

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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t i = min_param2-5; i <= max_param2+5; i++) {
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
    for (int8_t j = min_param2-5; j <= max_param2+5; j++) {
        sprintf(msg, "aIC_%03i", j);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                    cmd.primary, cmd.param1,
                    cmd.secondary, cmd.param2, cmd.flags);
        if (j > 0 && j <= 99) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
        for (int8_t j = min_param2-5; j <= max_param2+5; j++) {
            sprintf(msg, "aIC%i_%03i", i, j);
            cmd = SDI12Sensor::ParseCommand(msg, 'a');
            sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                        cmd.primary, cmd.param1,
                        cmd.secondary, cmd.param2, cmd.flags);
            if (i >= 1 && i <= 9 && j > 0 && j <= 99) {
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
    uint8_t min_param1 = 0;
    uint8_t max_param1 = 9;
    uint8_t min_param2 = 1;
    uint8_t max_param2 = 99;

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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
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
    for (int8_t j = min_param2-5; j <= max_param2+5; j++) {
        sprintf(msg, "aIR_%i", j);
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
        for (int8_t j = min_param2-5; j <= max_param2+5; j++) {
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
    for (int8_t i = min_param1-5; i <= max_param1+5; i++) {
        for (int8_t j = min_param2-5; j <= max_param2+5; j++) {
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
    TEST_ASSERT_EQUAL_MESSAGE(true, cmd.crc_requested, response);

    strcpy(msg, "aHB!");
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
        TEST_ASSERT_BITS_HIGH(CMD_HAS_META_FLAG | CMD_PARAM1_FLAG | CMD_PARAM2_FLAG, cmd.flags);
        TEST_ASSERT_BITS_LOW(CMD_PARAM_ERR_FLAG | CMD_IS_END_FLAG, cmd.flags);
    }

    /* Testing second parameter*/
    for (int8_t i = -2; i <= 10; i++) {
        sprintf(msg, "aXAB,U=%i,G=5!", i);
        cmd = SDI12Sensor::ParseCommand(msg, 'a');
        sprintf(response, "%s, c1=%i, p1=%i, c2=%i, p2=%i f=%X", msg,
                cmd.primary, cmd.param1,
                cmd.secondary, cmd.param2, cmd.flags);
        TEST_ASSERT_EQUAL_MESSAGE(kExtended, cmd.primary, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param1, response);
        TEST_ASSERT_EQUAL_MESSAGE(0, cmd.param2, response);
        TEST_ASSERT_BITS_LOW(
                CMD_IS_END_FLAG |
                CMD_PARAM1_FLAG |
                CMD_HAS_META_FLAG |
                CMD_PARAM2_FLAG |
                CMD_PARAM_ERR_FLAG |
                CMD_PARAM_SIGN_FLAG
                , cmd.flags);
    }
}


void test_parse_command_endptr() {
    char msg[20] = "aXAB,U=12,G=5!";
    SDI12CommandSet_s cmd;
    char *endptr;

    cmd = SDI12Sensor::ParseCommand(msg, 'a', &endptr);
    TEST_ASSERT_EQUAL_PTR(&msg[2], endptr);
    TEST_ASSERT_EQUAL_CHAR('A', *endptr);

    strcpy(msg, "aD1!");
    cmd = SDI12Sensor::ParseCommand(msg, 'a', &endptr);
    TEST_ASSERT_EQUAL_PTR(&msg[3], endptr);
    TEST_ASSERT_EQUAL_CHAR('!', *endptr);
    TEST_ASSERT_EQUAL(3, endptr - msg);

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
    RUN_TEST(test_parse_command_endptr);
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