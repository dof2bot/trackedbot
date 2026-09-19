/**
 * @file slew_limiter.h
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
 * @brief Discrete-time trapezoidal slew rate limiter for velocity ramping.
 *
 * Prevents mechanical gear shock and excessive battery current draw by limiting
 * the rate of change (acceleration/deceleration) of 8-bit PWM motor drive signals.
 *
 * Slew Rate Profile (Trapezoidal Velocity Ramping):
 *
 *     PWM Speed ^
 *               |               Target Steady-State (target_val)
 *   target_val -+---------------+------------------------+
 *               |              /                          \
 *               |             / (rate_per_step)            \ (rate_per_step)
 *               |            /                              \
 *               |           /                                \
 *               |          /                                  \
 *               |         /                                    \
 *               |        /                                      \
 *    start_val -+-------+----------------------------------------+---> Time (ticks)
 *               |   Acceleration                         Deceleration
 *               |     (Ramp-Up)                           (Ramp-Down)
 *
 * Mathematical Recurrence Relation:
 *  - If current_val < target_val: current_val = min(target_val, current_val + rate_per_step)
 *  - If current_val > target_val: current_val = max(target_val, current_val - rate_per_step)
 *  - If current_val == target_val: is_finished() == true
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "motion_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief State structure for discrete-time slew rate limiter.
 */
typedef struct {
  uint8_t current_val;   /**< Current instantaneous output value (PWM level 0..255). */
  uint8_t target_val;    /**< Target steady-state value to ramp towards. */
  uint8_t rate_per_step; /**< Maximum allowed step change per update cycle (> 0). */
} slew_limiter_t;

/**
 * @brief Initializes the slew rate limiter with starting value and slope rate.
 *
 * @param limiter Pointer to slew limiter instance.
 * @param initial_val Initial starting output and target value.
 * @param rate_per_step Maximum increment/decrement step per update tick (must be > 0).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if limiter is NULL or rate is 0.
 */
motion_status_t slew_limiter_init(slew_limiter_t *limiter, uint8_t initial_val, uint8_t rate_per_step);

/**
 * @brief Sets new target value for the limiter to ramp towards.
 *
 * @param limiter Pointer to slew limiter instance.
 * @param target New target value (0..255).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if limiter is NULL.
 */
motion_status_t slew_limiter_set_target(slew_limiter_t *limiter, uint8_t target);

/**
 * @brief Reconfigures the slope rate per update step.
 *
 * @param limiter Pointer to slew limiter instance.
 * @param rate_per_step Step rate per update tick (must be > 0).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if limiter is NULL or rate is 0.
 */
motion_status_t slew_limiter_set_rate(slew_limiter_t *limiter, uint8_t rate_per_step);

/**
 * @brief Advances current value one discrete step towards target.
 *
 * Applies clamp at target to prevent overshoot or oscillation.
 *
 * @param limiter Pointer to slew limiter instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if limiter is NULL.
 */
motion_status_t slew_limiter_update(slew_limiter_t *limiter);

/**
 * @brief Gets current instantaneous output value of the limiter.
 *
 * @param limiter Pointer to slew limiter instance.
 * @return uint8_t Current output value, or 0 if limiter is NULL.
 */
uint8_t slew_limiter_get_current(const slew_limiter_t *limiter);

/**
 * @brief Gets target value of the limiter.
 *
 * @param limiter Pointer to slew limiter instance.
 * @return uint8_t Target value, or 0 if limiter is NULL.
 */
uint8_t slew_limiter_get_target(const slew_limiter_t *limiter);

/**
 * @brief Checks if current value has reached target setpoint.
 *
 * @param limiter Pointer to slew limiter instance.
 * @return bool true if current_val == target_val (or limiter is NULL), false otherwise.
 */
bool slew_limiter_is_finished(const slew_limiter_t *limiter);

/**
 * @brief Forces immediate reset of current and target values, bypassing the ramp.
 *
 * Used for emergency stop, failsafe transitions, or mode resets.
 *
 * @param limiter Pointer to slew limiter instance.
 * @param value Value to force set immediately (0..255).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if limiter is NULL.
 */
motion_status_t slew_limiter_reset(slew_limiter_t *limiter, uint8_t value);

#ifdef __cplusplus
}
#endif
