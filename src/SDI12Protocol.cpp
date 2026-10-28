/**
 * @file SDI12Protocol.cpp
 * @copyright Stroud Water Research Center
 * @license This library is published under the BSD-3 license.
 *
 * @brief Implements shared, role-neutral SDI-12 protocol helpers.
 */

#include "SDI12Protocol.h"

bool SDI12Address::isValid(char address) {
  return (address >= '0' && address <= '9') || (address >= 'a' && address <= 'z') ||
    (address >= 'A' && address <= 'Z');
}

int8_t SDI12Address::toIndex(char address) {
  if (address >= '0' && address <= '9') { return address - '0'; }
  if (address >= 'a' && address <= 'z') { return address - 'a' + 10; }
  if (address >= 'A' && address <= 'Z') { return address - 'A' + 36; }
  return -1;
}

char SDI12Address::fromIndex(uint8_t index) {
  if (index < 10U) { return static_cast<char>(index + '0'); }
  if (index < 36U) { return static_cast<char>(index - 10U + 'a'); }
  if (index < kAddressCount) { return static_cast<char>(index - 36U + 'A'); }
  return '\0';
}
