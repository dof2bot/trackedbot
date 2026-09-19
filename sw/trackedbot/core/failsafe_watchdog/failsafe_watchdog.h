/**
 * @file failsafe_watchdog.h
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

#pragma once

#include "motion_types.h"
#include "tracked_motion.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default maximum allowed time between two commands.
 *
 * If the operator sends motion commands (e.g., streaming 10-20 Hz commands from
 * keyboard/joystick) or sends a periodic PING (heartbeat), the timer is constantly reset.
 * As soon as 1000 ms passes without a single packet, the failsafe is triggered.
 *
 * @brief Periodic interval in which the update function is executed.
 *
 * It should be as small as possible for the failsafe to react quickly,
 * but it must not be smaller than the time interval between update function calls.
 */
typedef enum {
  FAILSAFE_DEFAULT_TIMEOUT_MS = 1000,
  FAILSAFE_CHECK_INTERVAL_MS = 20
} failsafe_config_t;

/**
 * @brief Failsafe watchdog structure.
 *
 * @param tracked_motion Pointer to tracked_motion_t (module that controls the motors).
 * @param timeout_ms Maximum allowed silence in ms before stopping motors.
 * @param elapsed_since_feed_ms Time elapsed since last feed.
 * @param enabled Flag indicating if failsafe monitoring is enabled.
 * @param triggered Flag indicating if failsafe monitoring is triggered.
 */
typedef struct {
  tracked_motion_t *tracked_motion;
  uint32_t timeout_ms;
  uint32_t elapsed_since_feed_ms;
  bool enabled;
  bool triggered;
} failsafe_watchdog_t;

/**
 * @brief Initialize safety failsafe watchdog.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param tracked_motion Pointer to tracked_motion_t (module that controls the motors).
 * @param timeout_ms Maximum allowed silence in ms before stopping motors.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_init(failsafe_watchdog_t *watchdog, tracked_motion_t *tracked_motion, uint32_t timeout_ms);

/**
 * @brief Feed / reset the watchdog timer (called when a valid command arrives).
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_feed(failsafe_watchdog_t *watchdog);

/**
 * @brief Periodic time elapsed update.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param delta_ms Elapsed milliseconds since last update.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_update(failsafe_watchdog_t *watchdog, uint32_t delta_ms);

/**
 * @brief Enable or disable failsafe monitoring.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @param enable True to enable, false to disable.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t failsafe_enable(failsafe_watchdog_t *watchdog, bool enable);

/**
 * @brief Query if failsafe timeout was triggered.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return bool true if triggered, false otherwise.
 */
bool failsafe_is_triggered(const failsafe_watchdog_t *watchdog);

/**
 * @brief Query if failsafe monitoring is enabled.
 *
 * @param watchdog Pointer to failsafe_watchdog_t instance.
 * @return bool true if enabled, false otherwise.
 */
bool failsafe_is_enabled(const failsafe_watchdog_t *watchdog);

#ifdef __cplusplus
}
#endif
