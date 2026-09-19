/**
 * @file led_status.c
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
 * @brief Non-blocking diagnostic status LED driver for ATmega328P.
 *
 * Controls the onboard LED (Arduino Uno D13 / ATmega328P PB5) with visual
 * blinking patterns mapped to the robot's system operational mode.
 *
 * Hardware Feature (Single-Cycle Toggle):
 *  Writing a logical 1 to an AVR input register bit (PINB5) automatically toggles
 *  the corresponding output register bit (PORTB5) in a single clock cycle (62.5ns)
 *  without read-modify-write overhead.
 *
 * System Mode Blink Patterns Timing Chart:
 *
 * 1. SYSTEM_MODE_NORMAL (1 Hz Calm Heartbeat):
 *    100ms ON / 900ms OFF (Period T = 1000ms, Duty = 10%)
 *
 *    ON  +----+
 *        |    |
 *   OFF -+    +----------------------------------------------+----> Time (ms)
 *        |100 |<------------------- 900ms ------------------>|
 *        0   100                                            1000
 *
 * 2. SYSTEM_MODE_DEGRADED (5 Hz Fast Warning):
 *    100ms ON / 100ms OFF (Period T = 200ms, Duty = 50%)
 *
 *    ON  +----+    +----+    +----+    +----+    +----+
 *        |    |    |    |    |    |    |    |    |    |
 *   OFF -+    +----+    +----+    +----+    +----+    +----> Time (ms)
 *        0   100  200  300  400  500  600  700  800  900  1000
 *        |<-200ms->|
 *
 * 3. SYSTEM_MODE_PANIC (10 Hz Urgent Strobe):
 *    50ms ON / 50ms OFF (Period T = 100ms, Duty = 50%)
 *
 *    ON  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+
 *        | |  | |  | |  | |  | |  | |  | |  | |  | |  | |
 *   OFF -+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +----> Time (ms)
 *        0 50 100  200  300  400  500  600  700  800  900  1000
 *        |100ms|
 */

#include "led_status.h"
#include <avr/io.h>

/* LED pin D13 (Port B, Bit 5). */
static const uint8_t STATUS_LED_PIN_BIT = 5;

/* Last time (ms) when LED state was toggled. */
static uint32_t s_last_toggle_time = 0;

/* Current LED illumination state (true = ON, false = OFF). */
static bool s_led_state = false;

/**
 * @brief Configure PB5 as a digital output and extinguish LED initially.
 *
 * @return bool true on success false otherwise.
 */
bool led_status_init(void) {
  /* Configure PB5 as digital output. */
  DDRB |= (1 << STATUS_LED_PIN_BIT);

  /* Turn OFF initially. */
  PORTB &= ~(1 << STATUS_LED_PIN_BIT);

  /* Reset LED state. */
  s_led_state = false;

  /* Reset toggle time. */
  s_last_toggle_time = 0;

  return true;
}

/**
 * @brief Set the LED illumination state directly.
 *
 * @param on true to illuminate, false to extinguish.
 * @return bool true on success false otherwise.
 */
bool led_status_set(bool on) {
  s_led_state = on;

  if (on) {
    PORTB |= (1 << STATUS_LED_PIN_BIT);
  } else {
    PORTB &= ~(1 << STATUS_LED_PIN_BIT);
  }

  return true;
}

/**
 * @brief Toggle status LED in a single CPU cycle using PINB register.
 *
 * @return bool New LED illumination state (true if ON, false if OFF).
 */
bool led_status_toggle(void) {
  /* Writing a logical 1 to PINxn toggles the corresponding PORTxn register bit. */
  PINB = (1 << STATUS_LED_PIN_BIT);

  /* Update internal state flag. */
  s_led_state = !s_led_state;

  return s_led_state;
}

/**
 * @brief Non-blocking pattern generator evaluated in the main loop.
 *
 * Determines the required on/off intervals based on the current system mode,
 * compares against elapsed time, and toggles the LED when interval expires.
 *
 * Pattern behavior:
 * - SYSTEM_MODE_NORMAL: Heartbeat pulse (100ms ON every 1000ms).
 * - SYSTEM_MODE_DEGRADED: Fast alert blink (100ms ON / 100ms OFF).
 * - SYSTEM_MODE_PANIC: Continuous fast strobe (50ms ON / 50ms OFF).
 *
 * @param mode Current system operational mode (NORMAL, DEGRADED, PANIC).
 * @param current_time_ms Current millisecond timestamp from timer_avr_millis().
 * @return bool true if the LED state toggled during this call, false otherwise.
 */
bool led_status_update(system_mode_t mode, uint32_t current_time_ms) {
  uint16_t on_time;
  uint16_t off_time;

  switch (mode) {
  case SYSTEM_MODE_DEGRADED:
    /* Fast alert pattern (5 Hz): 100ms ON / 100ms OFF. */
    on_time = 100;
    off_time = 100;
    break;

  case SYSTEM_MODE_PANIC:
    /* Urgent strobe pattern (10 Hz): 50ms ON / 50ms OFF. */
    on_time = 50;
    off_time = 50;
    break;

  case SYSTEM_MODE_NORMAL:
  default:
    /* Calm heartbeat pulse (1 Hz): 100ms ON / 900ms OFF. */
    on_time = 100;
    off_time = 900;
    break;
  }

  /* Determine required interval based on current LED state. */
  uint32_t interval = (uint32_t)(s_led_state ? on_time : off_time);

  /* Check if it is time to toggle the LED. */
  if ((current_time_ms - s_last_toggle_time) >= interval) {
    /* Update toggle timer. */
    s_last_toggle_time = current_time_ms;

    /* Toggle LED state. */
    (void)led_status_toggle();

    return true;
  }

  return false;
}
