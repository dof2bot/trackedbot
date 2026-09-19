/**
 * @file shift_reg.h
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
 * @brief Low-level driver for 74HC595 8-bit Serial-In Parallel-Out (SIPO) shift register.
 *
 * Provides bit-banged SPI transmission to control the direction inputs of four
 * DC motor H-bridges on the Adafruit L293D Motor Shield v1 using only 4 MCU pins.
 *
 * Subsystem Topology:
 *   ATmega328P (4 pins)              74HC595 (8 outputs)              L293D H-Bridges
 *  +--------------------+            +-------------------+            +---------------+
 *  | Arduino D4  (PD4)  |--- CLK --->| SH_CP (Shift Clk) |            | M1 Direction  |
 *  | Arduino D7  (PD7)  |--- EN ---->| OE    (Out Enable)|--- Q0..Q7->| M2 Direction  |
 *  | Arduino D8  (PB0)  |--- SER --->| DS    (Data In)   |            | M3 Left Track |
 *  | Arduino D12 (PB4)  |--- LATCH-->| ST_CP (Latch Clk) |            | M4 Right Track|
 *  +--------------------+            +-------------------+            +---------------+
 *
 * Hardware Pin Mapping on ATmega328P (Arduino Uno):
 *  +-----------+--------+-------------+----------+---------------+------------------------------+
 *  | Signal    | 74HC595| Arduino Pin | AVR Port | Direction     | Function                     |
 *  +-----------+--------+-------------+----------+---------------+------------------------------+
 *  | DIR_CLK   | SH_CP  | Pin 4       | PD4      | Output        | Shift clock (rising edge)    |
 *  | DIR_EN    | OE     | Pin 7       | PD7      | Output (A-LOW)| Output enable (LOW = enable) |
 *  | DIR_SER   | DS     | Pin 8       | PB0      | Output        | Serial data input            |
 *  | DIR_LATCH | ST_CP  | Pin 12      | PB4      | Output        | Storage register latch clock |
 *  +-----------+--------+-------------+----------+---------------+------------------------------+
 *
 * 74HC595 Output Bit Allocation for Adafruit Motor Shield v1:
 *  +--------+--------+--------+--------+--------+--------+--------+--------+
 *  | Bit 7  | Bit 6  | Bit 5  | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
 *  | (Q7)   | (Q6)   | (Q5)   | (Q4)   | (Q3)   | (Q2)   | (Q1)   | (Q0)   |
 *  +--------+--------+--------+--------+--------+--------+--------+--------+
 *  |  M3_B  |  M4_B  |  M3_A  |  M2_B  |  M1_B  |  M1_A  |  M2_A  |  M4_A  |
 *  +--------+--------+--------+--------+--------+--------+--------+--------+
 *  Note: M3 controls Left Track (IC2), M4 controls Right Track (IC2).
 *
 * Serial Shift Timing Diagram:
 *  DIR_LATCH (PB4) ---\___________________________________________/--- (ST_CP)
 *                      Bit 7       Bit 6               Bit 0
 *  DIR_CLK   (PD4) ___/---\___/---\___/---\___ ... _____/---\________ (SH_CP)
 *  DIR_SER   (PB0) ===< D7  >===< D6  >=================< D0  >====== (DS)
 *  Outputs (Q0..Q7):  [Previous Latch State Held]        [New State Output]
 */

#pragma once

#include "motion_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure GPIO direction registers and initialize shift register.
 *
 * Configures PD4, PD7, PB0, and PB4 as digital outputs. Flushes an initial
 * 0x00 state to turn off all motor outputs, then asserts DIR_EN (LOW)
 * to enable the output drivers without spurious glitches.
 *
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t shift_reg_init(void);

/**
 * @brief Set or clear a specific bit in the internal shadow register.
 *
 * @note Does not immediately transmit to hardware. Call shift_reg_update()
 *       to latch the new values into the 74HC595.
 *
 * @param bit Output bit position (0 to 7).
 * @param value 0 to clear bit, non-zero to set bit.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if bit >= 8.
 */
motion_status_t shift_reg_set_bit(uint8_t bit, uint8_t value);

/**
 * @brief Read the current 8-bit latch shadow state.
 *
 * @return uint8_t Current shadow register byte.
 */
uint8_t shift_reg_get_state(void);

/**
 * @brief Shift the 8-bit shadow state into the 74HC595 and pulse the storage latch.
 *
 * Performs software SPI (bit-banging) shift operation from MSB (bit 7) down
 * to LSB (bit 0), followed by a rising edge on ST_CP (DIR_LATCH).
 *
 * Sequence:
 *  1. Pull DIR_LATCH (ST_CP) LOW to prepare storage register.
 *  2. For each bit from MSB (bit 7) down to LSB (bit 0):
 *     a. Pull DIR_CLK (SH_CP) LOW.
 *     b. Set DIR_SER (DS) to bit value.
 *     c. Pull DIR_CLK (SH_CP) HIGH (rising edge shifts data into register).
 *  3. Pull DIR_LATCH (ST_CP) HIGH (rising edge transfers data to output pins).
 *
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t shift_reg_update(void);

/**
 * @brief Atomically overwrite the shadow register and pulse the hardware latch.
 *
 * @param value 8-bit pattern to shift out immediately.
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t shift_reg_write(uint8_t value);

#ifdef __cplusplus
}
#endif
