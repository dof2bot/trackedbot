/**
 * @file tracked_motion.h
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
 * @brief Differential skid-steering kinematics and motion control for tracked platform.
 *
 * Manages dual-track differential locomotion, coordinating the left and right
 * motor handles with a trapezoidal slew rate limiter for acceleration and deceleration.
 *
 * Architecture Block Diagram:
 *  +-------------------------------------------------------------+
 *  |                      tracked_motion                         |
 *  |                                                             |
 *  |  +---------------------+        +-------------------------+ |
 *  |  |    slew_limiter     |        |    Differential State   | |
 *  |  |  (rate: 15 PWM/tick)|        |  (cmd, state, target)   | |
 *  |  +----------+----------+        +------------+------------+ |
 *  |             |                                |              |
 *  |             +---------------+----------------+              |
 *  |                             |                               |
 *  |              tracked_motion_apply_speeds()                  |
 *  +-----------------------------+-------------------------------+
 *                                |
 *            +-------------------+-------------------+
 *            |                                       |
 *            v                                       v
 *  +--------------------+                  +--------------------+
 *  |     left_motor     |                  |    right_motor     |
 *  |  (motor_handle_t)  |                  |  (motor_handle_t)  |
 *  +--------------------+                  +--------------------+
 *
 * Differential Skid-Steering Truth Table:
 *  +------------------+-----------------+------------------+------------------------------------+
 *  | Motion Command   | Left Track Dir  | Right Track Dir  | Kinematic Behavior                 |
 *  +------------------+-----------------+------------------+------------------------------------+
 *  | MOTION_FORWARD   | FORWARD (V_tgt) | FORWARD (V_tgt)  | Linear translation forward         |
 *  | MOTION_BACKWARD  | BACKWARD(V_tgt) | BACKWARD(V_tgt)  | Linear translation backward        |
 *  | MOTION_TURN_LEFT | STOP    (0)     | FORWARD (V_tgt)  | Pivot turn left around left track  |
 *  | MOTION_TURN_RIGHT| FORWARD (V_tgt) | STOP    (0)      | Pivot turn right around right track|
 *  | MOTION_SPIN_LEFT | BACKWARD(V_tgt) | FORWARD (V_tgt)  | In-place counter-clockwise spin    |
 *  | MOTION_SPIN_RIGHT| FORWARD (V_tgt) | BACKWARD(V_tgt)  | In-place clockwise spin            |
 *  | MOTION_STOP      | STOP    (0)     | STOP    (0)      | Dynamic active braking             |
 *  +------------------+-----------------+------------------+------------------------------------+
 */

#pragma once

#include "imotor.h"
#include "motion_types.h"
#include "slew_limiter.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Platform speed limits and default cruising setpoints (8-bit PWM).
 */
typedef enum {
  TRACKED_MOTION_SPEED_MIN = 60,      /**< Minimum PWM duty to overcome gearbox friction and avoid stall. */
  TRACKED_MOTION_SPEED_DEFAULT = 180, /**< Nominal cruising speed setpoint. */
  TRACKED_MOTION_SPEED_MAX = 255      /**< Maximum achievable PWM duty cycle (100%). */
} tracked_motion_speed_limit_t;

/**
 * @brief Context structure encapsulating tracked platform kinematics state.
 */
typedef struct {
  motor_handle_t left_motor;    /**< Abstract driver handle for left track motor (Channel 3). */
  motor_handle_t right_motor;   /**< Abstract driver handle for right track motor (Channel 4). */
  slew_limiter_t speed_limiter; /**< Slew rate limiter profile for smooth acceleration/braking. */
  uint8_t target_speed;         /**< Clamped target speed setpoint (60 - 255). */
  motion_cmd_t current_motion;  /**< Active differential motion command. */
  motion_state_t state;         /**< Platform operational state (IDLE, MOVING, FAILSAFE). */
} tracked_motion_t;

/**
 * @brief Initialize tracked mobile platform kinematics with motor handles.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param left Left track motor handle (imotor.h).
 * @param right Right track motor handle (imotor.h).
 * @param initial_speed Initial target speed (clamped between 60 and 255).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t
tracked_motion_init(tracked_motion_t *tm, motor_handle_t left, motor_handle_t right, uint8_t initial_speed);

/**
 * @brief Execute a motion command on the tracked chassis (skid-steering / differential).
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param cmd Motion command (FORWARD, BACKWARD, TURN_LEFT, SPIN_LEFT, etc.).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid arg or cmd.
 */
motion_status_t tracked_motion_set_command(tracked_motion_t *tm, motion_cmd_t cmd);

/**
 * @brief Set the target speed for subsequent or active motion commands.
 *
 * Clamps input between TRACKED_MOTION_SPEED_MIN (60) and TRACKED_MOTION_SPEED_MAX (255).
 * If currently moving, updates the target speed on the active slew limiter.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param speed Target speed value (clamped between MIN and MAX).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t tracked_motion_set_speed(tracked_motion_t *tm, uint8_t speed);

/**
 * @brief Periodic kinematics update routine (steps slew rate limiter and updates motor PWM).
 *
 * Must be called at regular intervals from the main loop or timer tick.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param delta_ms Elapsed milliseconds since last update.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t tracked_motion_update(tracked_motion_t *tm, uint16_t delta_ms);

/**
 * @brief Immediate stop of all motion, reset slew limiter and motors, transition to IDLE.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t tracked_motion_stop(tracked_motion_t *tm);

/**
 * @brief Get configured target speed setpoint.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return uint8_t Target speed, or 0 if tm is NULL.
 */
uint8_t tracked_motion_get_speed(const tracked_motion_t *tm);

/**
 * @brief Get current operational state.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_state_t Current state (IDLE, MOVING, FAILSAFE), or MOTION_STATE_IDLE if tm is NULL.
 */
motion_state_t tracked_motion_get_state(const tracked_motion_t *tm);

/**
 * @brief Get current active motion command.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_cmd_t Active command enum, or MOTION_STOP if tm is NULL.
 */
motion_cmd_t tracked_motion_get_command(const tracked_motion_t *tm);

/**
 * @brief Convert motion command enum to human-readable ASCII string (stored in Flash).
 *
 * @param cmd Motion command enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "FORWARD", "STOP", etc.).
 */
const char *tracked_motion_cmd_to_str(motion_cmd_t cmd);

/**
 * @brief Convert motion state enum to human-readable ASCII string (stored in Flash).
 *
 * @param state Motion state enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "IDLE", "MOVING", "FAILSAFE").
 */
const char *tracked_motion_state_to_str(motion_state_t state);

#ifdef __cplusplus
}
#endif
