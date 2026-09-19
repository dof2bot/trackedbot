/**
 * @file crc8.c
 * Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
 *
 * sr14_bot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sr14_bot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program_name. If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief Bitwise CRC-8 checksum calculator implementation.
 *
 * Uses the ATM/SMBus generator polynomial x^8 + x^2 + x^1 + x^0 (0x07)
 * to provide robust frame validation with low flash footprint and zero SRAM usage.
 */

#include "crc8.h"

enum {
  CRC8_POLYNOMIAL = 0x07
};

/**
 * @brief Update CRC-8 with a single data byte using polynomial 0x07 (ATM/SMBus).
 *
 * Polynomial: x^8 + x^2 + x^1 + 1 (0x07).
 *
 * @param crc Current CRC accumulator.
 * @param byte Next incoming byte.
 * @return uint8_t Updated CRC-8 value.
 */
uint8_t crc8_update(uint8_t crc, uint8_t byte) {
  crc ^= byte;

  for (uint8_t bit = 0; bit < 8; ++bit) {
    if (crc & 0x80) {
      crc = (uint8_t)((crc << 1) ^ CRC8_POLYNOMIAL);
    } else {
      crc = (uint8_t)(crc << 1);
    }
  }

  return crc;
}

/**
 * @brief Compute CRC-8 over a buffer of given length.
 *
 * @param data Pointer to data buffer.
 * @param length Number of bytes to process.
 * @return uint8_t Computed CRC-8 checksum.
 */
uint8_t crc8_calculate(const uint8_t *data, uint8_t length) {
  uint8_t crc = 0x00;

  if (data != (void *)0) {
    for (uint8_t i = 0; i < length; ++i) {
      crc = crc8_update(crc, data[i]);
    }
  }

  return crc;
}
