/**
 * @file timer_avr.c
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
 */

#include "timer_avr.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>

/**
 * @brief Localized CTC Compare Match Value:
 *        16MHz CPU clock / 64 prescaler / 1000Hz = 250 ticks -> OCR1A = 249
 *
 * @note CTC mode counts from 0 up to OCR1A. ISR increments the milliseconds
 *       counter every 1ms, so 1ms = 250 CPU cycles * 64 prescaler. HW clears
 *       the counter on compare match.
 *
 * TCNT1 (Counter)
 *   ^
 *   |
 *   |              OCR1A = 249  (Clear!)
 *   |             /|       /|        /|
 *   |            / |      / |       / |
 *   |           /  |     /  |      /  |
 *   |          /   |    /   |     /   |
 *   0 --------+----+---+----+----+----+---> Time (t)
 *                  ^        ^         ^
 *                  |        |         |
 *                 1ms      1ms       1ms (ISR: s_millis++)
 *
 */
static const uint16_t TIMER1_COMPARE_MATCH_1MS = 249;

/**
 * @brief Volatile 32-bit counter for elapsed milliseconds since boot.
 */
static volatile uint32_t s_millis = 0;

/**
 * @brief Timer1 Compare Match A Interrupt Service Routine.
 *
 *        Increments the 32-bit milliseconds counter every 1ms.
 *
 * @note When OCR1A reaches 249, the hardware clears TCNT1 and sets the
 *       OCF1A flag -> triggers ISR -> s_millis++.
 */
ISR(TIMER1_COMPA_vect) { s_millis++; }

/**
 * @brief Initialize Timer1 in CTC mode (Clear Timer on Compare Match).
 *
 *        16MHz CPU clock / 64 prescaler -> 250,000 cycles per second
 *        -> Compare Match interrupt every 1 millisecond.
 *
 * @note Zero-overhead timer (polling-based) for event-driven applications.
 *       This function must be called once during system initialization.
 *
 * @return bool true on successful timer initialization.
 */
bool timer_avr_init(void) {
  /* Stop Timer1 during setup */
  TCCR1A = 0;
  TCCR1B = 0;

  /* Reset timer counter */
  TCNT1 = 0;

  /* Set compare value for 1ms interval */
  OCR1A = TIMER1_COMPARE_MATCH_1MS;

  /* CTC mode (WGM12=1) with prescaler 64 (CS11=1, CS10=1) */
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);

  /* Enable Timer1 Compare Match A Interrupt */
  TIMSK1 |= (1 << OCIE1A);

  /* Reset milliseconds counter */
  s_millis = 0;

  return true;
}

/**
 * @brief Get elapsed milliseconds since system boot.
 *
 * @note Thread-safe non-blocking milliseconds counter (ISR-based).
 *
 * @return uint32_t Milliseconds counter.
 */
uint32_t timer_avr_millis(void) {
  uint32_t ms = 0;

  /* Atomic block with memory barrier prevents 32-bit tearing and compiler reordering. */
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { ms = s_millis; }

  return ms;
}

/**
 * @brief Check if a specified interval has elapsed since the last check.
 *
 * @note Thread-safe non-blocking interval timer (ISR-based).
 *       Avoids the use of delay functions (blocking code).
 *       Ideal for event-driven architectures.
 *
 * @param last_time Pointer to the variable holding the last timestamp.
 * @param interval_ms Target interval in milliseconds (uint32_t).
 * @return bool true if interval has elapsed (and updates *last_time),
 *              false otherwise.
 */
bool timer_avr_interval_elapsed(uint32_t *last_time, uint32_t interval_ms) {
  uint32_t current_time = timer_avr_millis();

  if ((current_time - *last_time) >= interval_ms) {
    *last_time = current_time;

    return true;
  }

  return false;
}
