#include <unity.h>
#include "test_dtoa.hpp"

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

#ifdef ARDUINO
void setup() {
    // NOTE!!! Wait for >2 secs
    // if board doesn't support software reset via Serial.DTR/RTS
    delay(2000);

    UNITY_BEGIN();

    process_dtoa_tests();

    UNITY_END();
}

void loop() {
    // Do nothing
}

#else

int main(int argc, char **argv) {
    if (argc || argv) {}
    UNITY_BEGIN();
    // process_tests();
    UNITY_END();
    return 0;
}

#endif