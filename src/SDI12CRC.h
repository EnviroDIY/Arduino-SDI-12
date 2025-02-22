/**
 * @file SDI12CRC.h
 *
 * @brief This file contains the main class to perform CRC calculations for
 * SDI12 as defined by SDI-12 Support Group Specification.
 * https://www.sdi-12.org/specification
 *
 */
#ifndef SDI12_CRC_H_
#define SDI12_CRC_H_

#include <stdint.h>
#include <stdbool.h>
// #include <stddef.h>

#define SDI12CRC_ASCII_LEN 3  // Number of ASCII char representation of CRC-16
#define SDI12CRC_ASCII_SIZE \
  4  // Size char array to store ASCII representation of CRC-16, includes null
     // terminator

class SDI12CRC {
 private:
  uint16_t crc_;
  char     ascii_[SDI12CRC_ASCII_SIZE] = "";

 public:
  static uint16_t Calculate(const char* str);
  static char*    crc16_to_acrc(uint16_t crc, char* str);
  static uint16_t acrc_to_crc16(const char* str);
  static bool     IsValid(const char* str);

  explicit SDI12CRC(void);
  explicit SDI12CRC(const char* str);
  explicit SDI12CRC(uint16_t crc);
  ~SDI12CRC(void);
  uint16_t    Get(void) const;
  const char* GetAscii(void) const;
  char*       Append(char* str);  // Append 3 ascii CRC-16 representation to string
  template<typename T>
  void Add(
    T value);  // Adds calculated CRC to existing, DO NOT use for string CRC calculation
};

/**
 * @brief Adds the CRC-16 calculation of integer types to existing calculated CRC,
 * DO NOT use for string CRC calculation.
 *
 * @tparam T Integer type parameter only, i.e int, long, double, float
 * @param value Value to calculate CRC
 *
 * Calculates CRC-16 and adds it to existing calculated CRC.
 * DO NOT USE for CRC-16 calculation of string. If required to calculate CRC
 * for a mix of string and integer types. Initialize SDI12CRC::SDI12CRC(const char *str)
 * first to calculate the crc from string then use this function to add subsequent
 * integer crc calculation.
 */
template<typename T>
void SDI12CRC::Add(T value) {
  for (size_t i = 0; i < sizeof(T); i++) {
    crc_ = crc_ ^ (value & 0xFF);
    for (int j = 0; j < 8; j++) {
      if ((crc_ & 0x0001)) {
        crc_ = crc_ >> 1;
        crc_ = crc_ ^ 0xA001;
      } else {
        crc_ = crc_ >> 1;
      }
    }
    value >>= 8;
  }
}

#endif
