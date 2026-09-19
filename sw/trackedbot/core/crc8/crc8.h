/**
 * @file crc8.h
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
 * @brief Hardware-friendly CRC-8 checksum calculation using ATM/SMBus polynomial standard.
 *
 * Provides bitwise CRC-8 calculation over arbitrary byte sequences and streaming
 * updates for binary protocol packet validation and telemetry generation.
 *
 * Algorithm Specification:
 *  - Polynomial:         x^8 + x^2 + x^1 + x^0 = 0x07 (ATM / SMBus)
 *  - Initial Value:      0x00
 *  - Input Reflection:   False (MSB-first processing)
 *  - Output Reflection:  False
 *  - Final XOR Value:    0x00
 *
 * Bitwise Shift & Feedback Diagram:
 *                  +----------------------------------------------+
 *                  |                                              |
 *  DATA Byte ----> (XOR) ----> [ Bit 7 ] ----> [ Bit 6..1 ] ----> [ Bit 0 ]
 *                    ^            |
 *                    |            v (if MSB was 1)
 *                    +------- (XOR 0x07)
 *
 * Verified Checksum Test Vectors:
 *  +--------------------------------+-----------------+---------------+
 *  | Input Data Stream (Bytes)      | Interpretation  | Result (HEX)  |
 *  +--------------------------------+-----------------+---------------+
 *  | "123456789" (ASCII 9 bytes)    | Standard test   | 0xF4          |
 *  | [0x04, 0x00]                   | CMD_PING        | 0x26          |
 *  | [0x03, 0x00]                   | CMD_STOP        | 0x2D          |
 *  | [0x84, 0x00]                   | RESP_PONG       | 0x60          |
 *  | [0x02, 0x01, 0x80]             | CMD_SET_SPEED   | 0x32          |
 *  +--------------------------------+-----------------+---------------+
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Update running CRC-8 accumulator with a single data byte.
 *
 * Evaluates polynomial feedback x^8 + x^2 + x^1 + 1 (0x07) over 8 bits.
 *
 * @param crc Current CRC accumulator state.
 * @param byte Next incoming byte from communication stream.
 * @return uint8_t Updated CRC-8 accumulator value.
 */
uint8_t crc8_update(uint8_t crc, uint8_t byte);

/**
 * @brief Compute CRC-8 checksum over a contiguous memory buffer.
 *
 * @param data Pointer to buffer containing packet data bytes.
 * @param length Number of bytes in buffer to process.
 * @return uint8_t Computed CRC-8 checksum (0x00 if data is NULL or length is 0).
 */
uint8_t crc8_calculate(const uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif
