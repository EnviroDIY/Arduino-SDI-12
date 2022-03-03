#ifdef ARDUINO
#include <Arduino.h>
#else
// #include <cstring>
// #include <cstdio>
// #include <ArduinoFake.h>
#endif
#include <SDI12CRC.h>
#include <unity.h>



// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

/* CRC SNIPPET */

char crc_ascii[4] = "";
char kresponse_as_input[] = "0+3.14+2.718+1.414";
char kexpect_crc_ascii[] = "Ipz";

// CRC unit tests
void test_crc_calculate_and_ascii_to_crc16(void) {
    uint16_t crc_from_string = SDI12CRC::Calculate(kresponse_as_input);
    uint16_t crc_from_ascii = SDI12CRC::acrc_to_crc16(kexpect_crc_ascii);
    TEST_ASSERT(crc_from_string);
    TEST_ASSERT(crc_from_ascii);
    TEST_ASSERT_EQUAL(crc_from_string, crc_from_ascii);
}

void test_crc_crc16_to_ascii(void) {
    uint16_t crc_from_ascii = SDI12CRC::acrc_to_crc16(kexpect_crc_ascii);
    char ascii[4] = "";
    SDI12CRC::crc16_to_acrc(crc_from_ascii, ascii);
    TEST_ASSERT_EQUAL_STRING(kexpect_crc_ascii, ascii);
}

void test_crc_void_constructor(void) {
    SDI12CRC crc;
    TEST_ASSERT_EQUAL_UINT16(0, crc.Get());
}

void test_crc_void_constructor_string(void) {
    SDI12CRC crc(kresponse_as_input);
    TEST_ASSERT_EQUAL_UINT16(SDI12CRC::Calculate(kresponse_as_input), crc.Get());
}

void test_crc_void_constructor_string_GetAscii(void) {
    SDI12CRC crc(kresponse_as_input);
    TEST_ASSERT_EQUAL_STRING(kexpect_crc_ascii, crc.GetAscii());
}

void test_crc_string_isvalid(void) {
    char response[] = "0+3.14+2.718+1.414Ipz\r\n";
    TEST_ASSERT(SDI12CRC::IsValid(response));
}

void test_crc_string_isvalid_noCRLF(void) {
    char response[] = "0+3.14+2.718+1.414Ipz";
    TEST_ASSERT(SDI12CRC::IsValid(response));
}

void test_crc_string_not_isvalid(void) {
    char response[] = "0+3.14+2.718+1.414\r\n";
    TEST_ASSERT_FALSE(SDI12CRC::IsValid(response));
}

static void process_crc_tests(void) {
    RUN_TEST(test_crc_calculate_and_ascii_to_crc16);
    RUN_TEST(test_crc_crc16_to_ascii);

    RUN_TEST(test_crc_void_constructor);
    RUN_TEST(test_crc_void_constructor_string);
    RUN_TEST(test_crc_void_constructor_string_GetAscii);
    
    RUN_TEST(test_crc_string_isvalid);
    RUN_TEST(test_crc_string_isvalid_noCRLF);
    RUN_TEST(test_crc_string_not_isvalid);
}
