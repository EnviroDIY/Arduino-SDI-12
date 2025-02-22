/**
 * @file SDI12CRC.cpp
 *
 * @brief This file contains the main class to perform CRC calculations for
 * SDI12 as defined by SDI-12 Support Group Specification.
 * https://www.sdi-12.org/specification
 *
 */
#include <string.h>
#include <stdlib.h>

#include "SDI12CRC.h"

/**
 * @brief Computes the 16-bit CRC value from string based on algorithm found in
 * paragraph 4.4.12.1 SDI-12 v1.4 specification.
 *
 * @param[in] str String to calculate crc from
 * @return uint16_t 16-bit CRC value
 */
uint16_t SDI12CRC::Calculate(const char* str) {
  uint16_t crc = 0x0000;
  for (size_t indx = 0; indx < strlen(str); indx++) {
    crc = crc ^ str[indx];
    for (int i = 0; i < 8; i++) {
      if ((crc & 0x0001)) {
        crc = crc >> 1;
        crc = crc ^ 0xA001;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc;
}

/**
 * @brief Converts 16-bit CRC into three ascii character representation as per
 * algorithm found in section 4.4.12.2 of SDI-12 v1.4 specification.
 *
 * @param[in] crc 16-bit CRC value
 * @param[out] str Array in memory where to store null-terminated string
 * @return char*
 */
char* SDI12CRC::crc16_to_acrc(uint16_t crc, char* str) {
  str[0] = (char)(0x0040 | (crc >> 12));
  str[1] = (char)(0x0040 | ((crc >> 6) & 0x003F));
  str[2] = (char)(0x0040 | (crc & 0x003F));
  str[3] = '\0';
  return str;
}

/**
 * @brief Computes the 16-bit CRC value from three CRC ascii characters. Algorithm
 * is the reverse as found from section 4.4.12.2 of SDI-12 v1.4 specification.
 *
 * @param[in] str Array in memory where null-terminated three ascii CRC character
 * representation is located.
 * @return uint16_t 16-bit representation of CRC
 */
uint16_t SDI12CRC::acrc_to_crc16(const char* str) {
  uint16_t crc = 0x0000;
  if (strlen(str) == SDI12CRC_ASCII_LEN) {
    crc |= ((str[0] & 0x00BF) << 12);
    crc |= ((str[1] & 0x00BF) << 6);
    crc |= (str[2] & 0x00BF);
  }
  return crc;
}

/**
 * @brief Computes and compare the appended 3-ASCII CRC-16 string with the string
 * to check string validity.
 *
 * @param[in] str String with 3-ASCII CRC-16 appended
 * @return true CRC-16 of string corresponds to CRC-16 of appended 3-ASCII CRC-16
 * @return false String size is equal to 3 or CRC-16 of string is not equal to
 * appended 3-ASCII CRC-16
 */
bool SDI12CRC::IsValid(const char* str) {
  uint16_t crc_from_ascii    = 0x0000;
  uint16_t crc_from_response = 0x0000;
  int      len               = strlen(str);

  if (len < SDI12CRC_ASCII_SIZE) return false;

  char* temp_str = (char*)malloc(len + 1);
  strcpy(temp_str, str);

  // Go to end end of string and strip out CR and LF
  char* end = strchr(temp_str, '\0');
  while (end[0] == '\r' || end[0] == '\n' || end[0] == '\0') {
    --end;
    end[1] = '\0';
  }
  len = ++end - temp_str;

  // Extract ASCII crc
  if (len > SDI12CRC_ASCII_LEN) {
    end               = end - SDI12CRC_ASCII_LEN;  // Move pointer to start of crc
    crc_from_ascii    = SDI12CRC::acrc_to_crc16(end);
    end[0]            = '\0';
    crc_from_response = SDI12CRC::Calculate(temp_str);
  }

  free(temp_str);
  if (crc_from_ascii != 0 && crc_from_ascii == crc_from_response) { return true; }
  return false;
}

/**
 * @brief Construct a new SDI12CRC::SDI12CRC object
 *
 */
SDI12CRC::SDI12CRC(void) {
  crc_ = 0x0000;
  memset(ascii_, '\0', SDI12CRC_ASCII_SIZE);
}

/**
 * @brief Construct a new SDI12CRC::SDI12CRC object
 *
 * @param[in] str String to calculate 16-bit CRC from
 */
SDI12CRC::SDI12CRC(const char* str) {
  crc_ = SDI12CRC::Calculate(str);
  SDI12CRC::crc16_to_acrc(crc_, ascii_);
}

/**
 * @brief Construct a new SDI12CRC::SDI12CRC object
 *
 * @param[in] crc 16-bit CRC
 */
SDI12CRC::SDI12CRC(uint16_t crc) {
  crc_ = crc;
  SDI12CRC::crc16_to_acrc(crc_, ascii_);
}

/**
 * @brief Destroy the SDI12CRC::SDI12CRC object
 *
 */
SDI12CRC::~SDI12CRC(void) {
  // Do nothing
}

/**
 * @brief Returns 16-bit CRC value
 *
 * @return uint16_t 16-bit CRC value
 */
uint16_t SDI12CRC::Get(void) const {
  return crc_;
}

/**
 * @brief Gets the 3-ASCII CRC character representation of CRC-16
 *
 * @return const char* Array in memory where null-terminated three ascii CRC character
 * representation is located.
 */
const char* SDI12CRC::GetAscii(void) const {
  return ascii_;
}

/**
 * @brief Appends 3-Char ASCII CRC-16 to string.
 *
 * @param[out] str Array in memory to append ascii representation of crc to.
 * Make sure char array is of appropriate size to append ascii crc to.
 * @return char* Array in memory where string with appended crc is located
 */
char* SDI12CRC::Append(char* str) {
  return strcat(str, ascii_);
}
