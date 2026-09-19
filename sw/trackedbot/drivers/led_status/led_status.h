/**
 * @file led_status.h
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
 * @brief Non-blocking diagnostic status LED pattern generator for ATmega328P.
 *
 * Drives the onboard status LED (Arduino Pin 13 / ATmega328P PB5) with discrete
 * optical cadence patterns reflecting the active system operating mode.
 *
 * Single-Cycle Hardware Toggle Feature:
 *  Writing a logical '1' to the AVR Input Pin register (PINB5) toggles the
 *  underlying PORTB5 output latch in a single clock cycle (62.5 ns @ 16 MHz)
 *  without requiring read-modify-write CPU instructions.
 *
 * System Mode Blink Patterns Timing Chart:
 *
 *  1. SYSTEM_MODE_NORMAL (1 Hz Calm Heartbeat):
 *     100ms ON / 900ms OFF (Period T = 1000ms, Duty Cycle = 10%)
 *     ON  +----+
 *         |    |
 *    OFF -+    +----------------------------------------------+----> Time (ms)
 *         |100 |<------------------- 900ms ------------------>|
 *         0   100                                            1000
 *
 *  2. SYSTEM_MODE_DEGRADED (2.5 Hz Warning Flash):
 *     100ms ON / 100ms OFF (Period T = 200ms, Duty Cycle = 50%)
 *     ON  +----+    +----+    +----+    +----+    +----+
 *         |    |    |    |    |    |    |    |    |    |
 *    OFF -+    +----+    +----+    +----+    +----+    +----> Time (ms)
 *         0   100  200  300  400  500  600  700  800  900  1000
 *         |<-200ms->|
 *
 *  3. SYSTEM_MODE_PANIC (10 Hz Urgent Strobe):
 *     50ms ON / 50ms OFF (Period T = 100ms, Duty Cycle = 50%)
 *     ON  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+  +-+
 *         | |  | |  | |  | |  | |  | |  | |  | |  | |  | |
 *    OFF -+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +--+ +----> Time (ms)
 *         0 50 100  200  300  400  500  600  700  800  900  1000
 *         |100ms|
 *
 * Pattern Summary Table:
 *  +-----------------------+---------+----------+-----------+------------+------------------------+
 *  | System Mode           | ON Time | OFF Time | Period T  | Frequency  | Visual Indication      |
 *  +-----------------------+---------+----------+-----------+------------+------------------------+
 *  | SYSTEM_MODE_NORMAL    | 100 ms  | 900 ms   | 1000 ms   | 1.0 Hz     | Healthy calm heartbeat |
 *  | SYSTEM_MODE_DEGRADED  | 100 ms  | 100 ms   | 200 ms    | 5.0 Hz     | Warning / degraded run |
 *  | SYSTEM_MODE_PANIC     | 50 ms   | 50 ms    | 100 ms    | 10.0 Hz    | Emergency stop strobe  |
 *  +-----------------------+---------+----------+-----------+------------+------------------------+
 */

#pragma once

#include "error_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configure PB5 as a digital output and extinguish LED initially.
 *
 * Sets DDRB bit 5 to configure Port B Pin 5 as digital output, clears PORTB
 * bit 5 to turn off the LED, and initializes internal timing state.
 *
 * @return bool true on success, false otherwise.
 */
bool led_status_init(void);

/**
 * @brief Set the LED illumination state directly.
 *
 * @param on true to illuminate LED (HIGH), false to extinguish LED (LOW).
 * @return bool true on success, false otherwise.
 */
bool led_status_set(bool on);

/**
 * @brief Toggle status LED in a single CPU cycle using the PINB register.
 *
 * Writes logical 1 to PINB5 to execute a single-cycle hardware toggle.
 *
 * @return bool New LED illumination state (true if ON, false if OFF).
 */
bool led_status_toggle(void);

/**
 * @brief Non-blocking pattern generator evaluated periodically in superloop.
 *
 * Selects ON/OFF interval boundaries mapped to the current system mode, compares
 * against current millisecond time, and toggles the LED when interval elapses.
 *
 * @param mode Current system operational mode (NORMAL, DEGRADED, PANIC).
 * @param current_time_ms Current millisecond timestamp from timer_avr_millis().
 * @return bool true if the LED state toggled during this call, false otherwise.
 */
bool led_status_update(system_mode_t mode, uint32_t current_time_ms);

#ifdef __cplusplus
}
#endif
