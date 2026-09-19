/**
 * @file shift_reg.c
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
 * @brief Driver for the 74HC595 8-bit serial-in parallel-out shift register.
 *
 * The L293D Motor Shield uses an SN74HC595N shift register to control the
 * direction pins of four DC motor H-bridges using only four microcontroller pins.
 *
 *   ATmega328P (4 pins)              74HC595 (8 outputs)              L293D H-Bridges
 * +--------------------+            +-------------------+            +---------------+
 * | Arduino D4  (PD4)  |--- CLK --->| SH_CP (Shift Clk) |            | M1 Direction  |
 * | Arduino D7  (PD7)  |--- EN ---->| OE    (Out Enable)|--- Q0..Q7->| M2 Direction  |
 * | Arduino D8  (PB0)  |--- SER --->| DS    (Data In)   |            | M3 Left Track |
 * | Arduino D12 (PB4)  |--- LATCH-->| ST_CP (Latch Clk) |            | M4 Right Track|
 * +--------------------+            +-------------------+            +---------------+
 *
 * Hardware Wiring on Arduino Uno (ATmega328P):
 *  - DIR_CLK   (SH_CP): Arduino Pin 4  -> Port D, Bit 4 (PD4)
 *  - DIR_EN    (OE):    Arduino Pin 7  -> Port D, Bit 7 (PD7, Active-LOW)
 *  - DIR_SER   (DS):    Arduino Pin 8  -> Port B, Bit 0 (PB0)
 *  - DIR_LATCH (ST_CP): Arduino Pin 12 -> Port B, Bit 4 (PB4)
 *
 * 74HC595 Serial Shift Timing Diagram:
 *
 * DIR_LATCH (PB4) ---\___________________________________________/--- (ST_CP)
 *                     Bit 7       Bit 6               Bit 0
 * DIR_CLK   (PD4) ___/---\___/---\___/---\___ ... _____/---\________ (SH_CP)
 * DIR_SER   (PB0) ===< D7  >===< D6  >=================< D0  >====== (DS)
 * Outputs (Q0..Q7):  [Previous Latch State Held]        [New State Output]
 *
 * 74HC595 Output Bit Allocation for L293D H-Bridges:
 *  +--------+--------+--------+--------+--------+--------+--------+--------+
 *  | Bit 7  | Bit 6  | Bit 5  | Bit 4  | Bit 3  | Bit 2  | Bit 1  | Bit 0  |
 *  |  M3_B  |  M4_B  |  M3_A  |  M2_B  |  M1_B  |  M1_A  |  M2_A  |  M4_A  |
 *  +--------+--------+--------+--------+--------+--------+--------+--------+
 *  (M3: Left Track on IC2, M4: Right Track on IC2)
 */

#include "shift_reg.h"
#include <avr/io.h>

/* Port D pins: CLK (Pin 4 -> PD4), EN (Pin 7 -> PD7). */
static const uint8_t SHIFT_REG_CLK_BIT = PORTD4;
static const uint8_t SHIFT_REG_EN_BIT = PORTD7;

/* Port B pins: SER (Pin 8 -> PB0), LATCH (Pin 12 -> PB4). */
static const uint8_t SHIFT_REG_SER_BIT = PORTB0;
static const uint8_t SHIFT_REG_LATCH_BIT = PORTB4;

/**
 * @brief In-memory shadow register tracking the 8-bit latch state.
 *
 * @note Since the 74HC595 shift register does not have readback capability,
 *       this variable maintains the current state of the outputs to avoid
 *       unexpected behavior when updating individual bits.
 */
static uint8_t s_latch_state = 0;

/**
 * @brief Configure GPIO direction registers and initialize shift register.
 *
 * Configures PD4, PD7, PB0, and PB4 as digital outputs. Flushes an initial
 * 0x00 state to turn off all motor outputs, then asserts DIR_EN (LOW)
 * to enable the output drivers.
 *
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t shift_reg_init(void) {
  /* Configure control pins as outputs on Port D and Port B. */
  DDRD |= (uint8_t)((1 << SHIFT_REG_CLK_BIT) | (1 << SHIFT_REG_EN_BIT));
  DDRB |= (uint8_t)((1 << SHIFT_REG_SER_BIT) | (1 << SHIFT_REG_LATCH_BIT));

  /* Clear outputs and reset shift register (0x00), turn off all motors. */
  s_latch_state = 0;
  (void)shift_reg_update();

  /* Enable 74HC595 outputs (Active LOW on DIR_EN). */
  PORTD &= (uint8_t)(~(1 << SHIFT_REG_EN_BIT));

  /**
   * @brief Critical order: Must enable outputs after updating with 0x00,
   *        otherwise the outputs will be garbage during transition.
   *        For more details see the Fritzing comments at the top.
   */

  return MOTION_STATUS_OK;
}

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
motion_status_t shift_reg_set_bit(uint8_t bit, uint8_t value) {
  if (bit >= 8) {
    return MOTION_STATUS_INVALID_ARG;
  }

  if (value != 0) {
    s_latch_state |= (uint8_t)(1 << bit);
  } else {
    s_latch_state &= (uint8_t)(~(1 << bit));
  }

  return MOTION_STATUS_OK;
}

/**
 * @brief Read the current 8-bit latch shadow state.
 *
 * @return uint8_t Current shadow register byte.
 */
uint8_t shift_reg_get_state(void) { return s_latch_state; }

/**
 * @brief Shift the 8-bit shadow state into the 74HC595 and pulse the storage latch.
 *
 * @note This function updates the 74HC595 hardware registers with the current
 *       state of the shadow register. Performs software SPI (bit-banging) shift operation.
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
motion_status_t shift_reg_update(void) {
  int8_t i;

  /* Pull Latch LOW to start transmission (Port B, Pin 12). */
  PORTB &= (uint8_t)(~(1 << SHIFT_REG_LATCH_BIT));

  /**
   * @brief Shift out 8 bits, MSB first (bit 7 down to 0).
   * @note  While the loop runs for i = 7..0, the internal data is shifted,
   *        but the chip outputs (Q0..Q7) remain frozen in the old state.
   *        Only on step 3, when LATCH jumps from 0 to 1, all 8 outputs
   *        are updated simultaneously, preventing any false motor jerks
   *        during transmission.
   */
  for (i = 7; i >= 0; i--) {
    /* Clock LOW (Port D, Pin 4). */
    PORTD &= (uint8_t)(~(1 << SHIFT_REG_CLK_BIT));

    /* Set Serial Data pin (Port B, Pin 8). */
    if ((s_latch_state & (1 << i)) != 0) {
      PORTB |= (uint8_t)(1 << SHIFT_REG_SER_BIT);
    } else {
      PORTB &= (uint8_t)(~(1 << SHIFT_REG_SER_BIT));
    }

    /* Clock HIGH to shift bit into storage. */
    PORTD |= (uint8_t)(1 << SHIFT_REG_CLK_BIT);
  }

  /* Pull Latch HIGH to transfer shifted bits to output pins. */
  PORTB |= (uint8_t)(1 << SHIFT_REG_LATCH_BIT);

  return MOTION_STATUS_OK;
}

/**
 * @brief Atomically overwrite the shadow register and pulse the hardware latch.
 *
 * @param value 8-bit pattern to shift out immediately.
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t shift_reg_write(uint8_t value) {
  s_latch_state = value;

  return shift_reg_update();
}
