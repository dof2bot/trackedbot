/**
 * @file failsafe_watchdog.c
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
 * @brief Communication safety failsafe watchdog and heartbeat monitor.
 *
 * Monitors command link activity and automatically stops robot motion if the
 * control link drops or no valid command packets arrive within the timeout window.
 *
 * Dual Watchdog Architecture Comparison:
 *  +--------------------+----------------------------+-------------------------------+
 *  | Feature / Aspect   | Hardware Watchdog (WDT)    | Software Failsafe Watchdog    |
 *  +--------------------+----------------------------+-------------------------------+
 *  | Implementation     | Silicon peripheral (AVR)   | Application layer (core)      |
 *  | Protection Target  | Firmware freeze / Deadlock | Comm link loss (WiFi/UART)    |
 *  | Timeout Threshold  | 2000 ms (WDTO_2S)          | 1000 ms (FAILSAFE_DEFAULT)    |
 *  | Feeding Mechanism  | wdt_reset() in superloop   | failsafe_feed() on valid RX   |
 *  | Expiration Action  | Hard MCU system reset      | tracked_motion_stop()         |
 *  | System State After | Reboot into bootloader     | State -> MOTION_STATE_FAILSAFE|
 *  +--------------------+----------------------------+-------------------------------+
 *
 * Failsafe Heartbeat & Timeout Timing Diagram:
 *
 *  Valid Cmds (RX) --[Feed]------[Feed]------[Feed]---------------------------->
 *                       |<--ok-->|   |<--ok-->|       |
 *  Elapsed Timer    0---+--------0---+--------0-------+-----> 1000 ms (TIMEOUT)
 *                                                     |
 *  Motors State     [       DRIVING / MOVING         ]|[ EMERGENCY STOP / BRAKE ]
 *  Motion State     [       MOTION_STATE_MOVING      ]|[ MOTION_STATE_FAILSAFE  ]
 *  Triggered Flag   false -----------------------------> true
 *
 * State Transition Flow:
 *
 *       +---------------------------------------------+
 *       |             MOTION_STATE_STOPPED            |<--------+
 *       +---------------------------------------------+         |
 *             |                                                 |
 *       (Motion Cmd)                                       (Clear Err /
 *             v                                             Valid Feed)
 *       +---------------------------------------------+         |
 *  +--->|             MOTION_STATE_MOVING             |         |
 *  |    +---------------------------------------------+         |
 *  |          |                                                 |
 *  |    (Silence >= 1000ms)                                     |
 *  |          v                                                 |
 *  |    +---------------------------------------------+         |
 *  |    |             tracked_motion_stop()           |         |
 *  |    |         State -> MOTION_STATE_FAILSAFE      |---------+
 *  |    +---------------------------------------------+
 *  |          |
 *  +----------+ (New Motion Command Received)
 */

#include "failsafe_watchdog.h"

/**
 * @brief Initialize safety failsafe watchdog.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param tracked_motion Pointer to tracked_motion_t (module that controls the motors).
 * @param timeout_ms Maximum allowed silence in ms before stopping motors.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_init(failsafe_watchdog_t *watchdog, tracked_motion_t *tracked_motion, uint32_t timeout_ms) {
  if (watchdog == (void *)0 || tracked_motion == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  watchdog->tracked_motion = tracked_motion;
  watchdog->timeout_ms = timeout_ms;
  watchdog->elapsed_since_feed_ms = 0;
  watchdog->enabled = true;
  watchdog->triggered = false;

  return MOTION_STATUS_OK;
}

/**
 * @brief Feed / reset the watchdog timer (called when a valid command arrives).
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_feed(failsafe_watchdog_t *watchdog) {
  if (watchdog == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  watchdog->elapsed_since_feed_ms = 0;
  watchdog->triggered = false;

  return MOTION_STATUS_OK;
}

/**
 * @brief Periodic time elapsed update.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param delta_ms Elapsed milliseconds since last update.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_update(failsafe_watchdog_t *watchdog, uint32_t delta_ms) {
  if (watchdog == (void *)0 || watchdog->tracked_motion == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  if (!watchdog->enabled) {
    return MOTION_STATUS_OK;
  }

  watchdog->elapsed_since_feed_ms += delta_ms;

  if (watchdog->elapsed_since_feed_ms >= watchdog->timeout_ms) {
    if (!watchdog->triggered) {
      if (tracked_motion_get_state(watchdog->tracked_motion) == MOTION_STATE_MOVING) {
        (void)tracked_motion_stop(watchdog->tracked_motion);
        watchdog->tracked_motion->state = MOTION_STATE_FAILSAFE;
      }
      watchdog->triggered = true;
    }
  }

  return MOTION_STATUS_OK;
}

/**
 * @brief Enable or disable failsafe monitoring.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param enable True to enable, false to disable.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_enable(failsafe_watchdog_t *watchdog, bool enable) {
  if (watchdog == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  watchdog->enabled = enable;

  if (!enable) {
    watchdog->elapsed_since_feed_ms = 0;
    watchdog->triggered = false;
  }

  return MOTION_STATUS_OK;
}

/**
 * @brief Query if failsafe timeout was triggered.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return bool true if triggered, false otherwise.
 */
bool failsafe_is_triggered(const failsafe_watchdog_t *watchdog) {
  if (watchdog == (void *)0) {
    return false;
  }

  return (watchdog->triggered != false);
}

/**
 * @brief Query if failsafe monitoring is enabled.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return bool true if enabled, false otherwise.
 */
bool failsafe_is_enabled(const failsafe_watchdog_t *watchdog) {
  if (watchdog == (void *)0) {
    return false;
  }

  return (watchdog->enabled != false);
}
