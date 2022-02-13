#ifdef ARDUINO
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <SDI12Sensor.h>
#include <Arduino.h>
#else
#include <cstring>
#include <cstdio>
// #include <ArduinoFake.h>
// #include <StreamFake.h>
#endif
#include <unity.h>

void test_dtoa_basic_nolimit(void) {
    char stringValue[20];
    double value = +123.5678;
    int num;
    num = dtoa(value, stringValue, 6);
    TEST_ASSERT_TRUE(num >= 9);
    value *=-1;
    num = dtoa(value, stringValue, 6);
    TEST_ASSERT_TRUE(num >= 9);
}

void test_dtoa_sign(void) {
    char stringValue[20];
    // Int test
    dtoa(123, stringValue, 6);
    TEST_ASSERT_TRUE(stringValue[0] == '+');
    dtoa(-123, stringValue, 6);
    TEST_ASSERT_TRUE(stringValue[0] == '-');
    //float test
    double value = 1.345;
    dtoa(value, stringValue, 6);
    TEST_ASSERT_TRUE(stringValue[0] == '+');
    dtoa(-value, stringValue, 6);
    TEST_ASSERT_TRUE(stringValue[0] == '-');
}

void test_dtoa_no_pos_sign_no_limit(void) {
    char stringValue[20];
    // Int test
    dtoa(123, stringValue, 6, 0, false, false);
    TEST_ASSERT_TRUE(stringValue[0] == '1');
    dtoa(-123, stringValue, 6,0, false, false);
    TEST_ASSERT_TRUE(stringValue[0] == '-');
}

void test_dtoa_decimal(void) {
    char stringValue[20];
    double value = 1.01;
    dtoa(value, stringValue, 2);
    TEST_ASSERT_EQUAL_STRING("+1.01", stringValue);
}

const double kPrecisionTestVal = 0.105;

void test_dtoa_precision(void) {
    char stringValue[20];
    double value = -0.123456789;
    int num;
    num = dtoa(value, stringValue, 6);
    TEST_ASSERT_EQUAL(9, num);
}

void test_dtoa_0_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 0);
    TEST_ASSERT_EQUAL_STRING("+0", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 0);
    TEST_ASSERT_EQUAL_STRING("-0", stringValue);
}

void test_dtoa_1_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 1);
    TEST_ASSERT_EQUAL_STRING("+0.1", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 1);
    TEST_ASSERT_EQUAL_STRING("-0.1", stringValue);
}

void test_dtoa_2_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 2);
    TEST_ASSERT_EQUAL_STRING("+0.11", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 2);
    TEST_ASSERT_EQUAL_STRING("-0.11", stringValue);
}

void test_dtoa_3_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 3);
    TEST_ASSERT_EQUAL_STRING("+0.105", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 3);
    TEST_ASSERT_EQUAL_STRING("-0.105", stringValue);
}

void test_dtoa_4_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 4);
    TEST_ASSERT_EQUAL_STRING("+0.105", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 4);
    TEST_ASSERT_EQUAL_STRING("-0.105", stringValue);
}

void test_dtoa_5_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 5);
    TEST_ASSERT_EQUAL_STRING("+0.105", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 5);
    TEST_ASSERT_EQUAL_STRING("-0.105", stringValue);
}

void test_dtoa_6_precision(void) {
    char stringValue[20];
    double value = kPrecisionTestVal;
    // Test positive case
    dtoa(value, stringValue, 6);
    TEST_ASSERT_EQUAL_STRING("+0.105", stringValue);
    // Test negative case
    dtoa(-value, stringValue, 6);
    TEST_ASSERT_EQUAL_STRING("-0.105", stringValue);
}

void test_dtoa_string_prec_no_trail_zero(void) {
    char stringValue[20];
    double value = -0.1000;
    int num;
    num = dtoa(value, stringValue, 6);
    TEST_ASSERT_EQUAL(4, num);
    TEST_ASSERT_EQUAL_STRING("-0.1", stringValue);
}

void test_dtoa_string_prec_yes_trail_zero(void) {
    char stringValue[20];
    double value = -1.0;
    int num;
    num = dtoa(value, stringValue, 2, 0, true);
    TEST_ASSERT_EQUAL_STRING("-1.00", stringValue);
    TEST_ASSERT_EQUAL(5, num);
}

void test_dtoa_whole_gt_max_length(void) {
    char stringValue[20];
    double value = 123.56789;
    char expected_str[] = "NaN";
    int num;
    num = dtoa(value, stringValue, 6, 3);
    TEST_ASSERT_EQUAL_STRING(expected_str, stringValue);
    TEST_ASSERT_EQUAL(0, num);
}

void test_dtoa_prec_gt_fitlen(void) {
    char stringValue[20];
    double value = 1.3456789;
    uint8_t max_len = 6; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 8, max_len);
    TEST_ASSERT_EQUAL(max_len, num);
}


void test_dtoa_prec_eq_fitlen(void) {
    char stringValue[20];
    double value = 0.3456789;
    uint8_t max_len = 6; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 6, max_len);
    TEST_ASSERT_EQUAL_STRING("+0.346", stringValue);
    TEST_ASSERT_EQUAL(max_len, num);
}

void test_dtoa_prec_lt_fitlen(void) {
    char stringValue[20];
    double value = +1.12345678;
    uint8_t max_len = 9; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 3, max_len);
    TEST_ASSERT_EQUAL_STRING("+1.123", stringValue);
    TEST_ASSERT_EQUAL(6, num);
}


void test_dtoa_no_pos_sign_prec_gt_tofit(void) {
    char stringValue[20];
    double value = 1.345678;
    uint8_t max_len = 6; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 8, max_len, false, false);
    TEST_ASSERT_TRUE(stringValue[0] == '1');
    TEST_ASSERT_EQUAL_STRING("1.3457", stringValue);
    TEST_ASSERT_EQUAL(6, num);
}

void test_dtoa_no_pos_sign_prec_lt_tofit(void) {
    char stringValue[20];
    double value = 1.123456;
    uint8_t max_len = 9; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 3, max_len, false, false);
    TEST_ASSERT_TRUE(stringValue[0] == '1');
    TEST_ASSERT_EQUAL_STRING("1.123", stringValue);
    TEST_ASSERT_EQUAL(5, num);
}

void test_dtoa_negval_no_pos_sign_prec_gt_tofit(void) {
    char stringValue[20];
    double value = -2.456789;
    uint8_t max_len = 6; // includes sign and decimal
    int num;
    num = dtoa(value, stringValue, 8, max_len);
    TEST_ASSERT_TRUE(stringValue[0] == '-');
    TEST_ASSERT_EQUAL_STRING("-2.457", stringValue);
    TEST_ASSERT_EQUAL(max_len, num);
}


void test_integral_len(void) {
    float val = 0.56789;
    TEST_ASSERT(IntegralLength(val));
    TEST_ASSERT_EQUAL(1, IntegralLength(val));
    val = 123.56789;
    TEST_ASSERT_EQUAL(3, IntegralLength(val));
}

static void process_dtoa_tests(void) {
    RUN_TEST(test_integral_len);

    RUN_TEST(test_dtoa_basic_nolimit);
    RUN_TEST(test_dtoa_sign);
    RUN_TEST(test_dtoa_no_pos_sign_no_limit);
    RUN_TEST(test_dtoa_decimal);
    RUN_TEST(test_dtoa_precision);
    RUN_TEST(test_dtoa_0_precision);
    RUN_TEST(test_dtoa_1_precision);
    RUN_TEST(test_dtoa_2_precision);
    RUN_TEST(test_dtoa_3_precision);
    RUN_TEST(test_dtoa_4_precision);
    RUN_TEST(test_dtoa_5_precision);
    RUN_TEST(test_dtoa_6_precision);
    RUN_TEST(test_dtoa_string_prec_no_trail_zero);
    RUN_TEST(test_dtoa_string_prec_yes_trail_zero);
    RUN_TEST(test_dtoa_whole_gt_max_length);
    RUN_TEST(test_dtoa_prec_gt_fitlen);
    RUN_TEST(test_dtoa_prec_eq_fitlen);
    RUN_TEST(test_dtoa_prec_lt_fitlen);
    RUN_TEST(test_dtoa_no_pos_sign_prec_gt_tofit);
    RUN_TEST(test_dtoa_no_pos_sign_prec_lt_tofit);
    RUN_TEST(test_dtoa_negval_no_pos_sign_prec_gt_tofit);
}
