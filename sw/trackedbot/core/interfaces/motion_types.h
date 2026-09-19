/**
 * @file motion_types.h
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
 * @brief Common kinematic and motion control data types for tracked chassis.
 *
 * Defines high-level motion command enums, vehicle operational states, and
 * status return codes used across core kinematics and motor hardware adapters.
 *
 * Track Kinematics Truth Table (Differential Skid-Steering):
 *  +--------------------+----------------+-----------------+---------------------------+
 *  | Motion Command     | Left Track     | Right Track     | Maneuver Type             |
 *  +--------------------+----------------+-----------------+---------------------------+
 *  | MOTION_STOP        | STOP (PWM=0)   | STOP (PWM=0)    | Full Stop / Standstill    |
 *  | MOTION_FORWARD     | FORWARD        | FORWARD         | Straight Forward Drive    |
 *  | MOTION_BACKWARD    | BACKWARD       | BACKWARD        | Straight Reverse Drive    |
 *  | MOTION_TURN_LEFT   | STOP           | FORWARD         | Pivot Turn (Radius = W)   |
 *  | MOTION_TURN_RIGHT  | FORWARD        | STOP            | Pivot Turn (Radius = W)   |
 *  | MOTION_SPIN_LEFT   | BACKWARD       | FORWARD         | In-Place Zero-Radius Spin |
 *  | MOTION_SPIN_RIGHT  | FORWARD        | BACKWARD        | In-Place Zero-Radius Spin |
 *  +--------------------+----------------+-----------------+---------------------------+
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief High-level motion commands for tracked mobile robot chassis.
 */
typedef enum {
  MOTION_STOP = 0,   /**< All motors stopped and dynamic brake engaged. */
  MOTION_FORWARD,    /**< Drive straight forward (both tracks forward). */
  MOTION_BACKWARD,   /**< Drive straight backward (both tracks reverse). */
  MOTION_TURN_LEFT,  /**< Pivot turn left (left track stopped, right track forward). */
  MOTION_TURN_RIGHT, /**< Pivot turn right (right track stopped, left track forward). */
  MOTION_SPIN_LEFT,  /**< In-place spin left (left reverse, right forward). */
  MOTION_SPIN_RIGHT  /**< In-place spin right (left forward, right reverse). */
} motion_cmd_t;

/**
 * @brief Operational motion states of the vehicle chassis.
 */
typedef enum {
  MOTION_STATE_IDLE = 0, /**< Vehicle at standstill (motors stopped). */
  MOTION_STATE_MOVING,   /**< Vehicle actively driving or ramping speeds. */
  MOTION_STATE_FAILSAFE  /**< Vehicle emergency stopped due to comm timeout. */
} motion_state_t;

/**
 * @brief Standardized return codes for motion and actuator operations.
 */
typedef enum {
  MOTION_STATUS_OK = 0,     /**< Operation succeeded without errors. */
  MOTION_STATUS_ERROR,      /**< Hardware or execution error occurred. */
  MOTION_STATUS_INVALID_ARG /**< Invalid argument provided (e.g. NULL pointer). */
} motion_status_t;

#ifdef __cplusplus
}
#endif
