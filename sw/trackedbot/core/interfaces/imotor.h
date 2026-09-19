/**
 * @file imotor.h
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
 * @brief Abstract DC motor controller interface (Output Port / DIP).
 *
 * Defines the actuator abstraction conforming to the Dependency Inversion
 * Principle (DIP) in Clean Architecture. Decouples the core kinematics engine
 * (tracked_motion) from concrete motor driver hardware (L293D shield, L298N,
 * TB6612, or unit-test mocks).
 *
 * Ports & Adapters Architectural Flow:
 *
 *    +-------------------------------+
 *    |        tracked_motion         |   [ CORE / KINEMATICS LAYER ]
 *    +-------------------------------+
 *                   |
 *                   v (Depends strictly on interface)
 *    =================================
 *    MOTOR PORT: motor_handle_t (imotor.h)
 *      - set_direction()
 *      - set_speed()
 *      - stop()
 *    =================================
 *                   ^
 *                   | (Implements adapter)
 *    +-------------------------------+
 *    |         l293d_shield          |   [ ADAPTERS / HARDWARE LAYER ]
 *    +-------------------------------+
 */

#pragma once

#include "motion_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Motor rotation directions for H-bridge drivers.
 */
typedef enum {
  MOTOR_DIR_STOP = 0,
  MOTOR_DIR_FORWARD,
  MOTOR_DIR_BACKWARD
} motor_dir_t;

/**
 * @brief Abstract DC motor controller interface (Output Port / DIP).
 *
 * @param context Pointer to driver-specific motor context (e.g. channel, inversion flag).
 * @param set_direction Function pointer to set H-bridge direction.
 * @param set_speed Function pointer to set PWM speed (0 - 255).
 * @param stop Function pointer to immediately stop the motor.
 */
typedef struct {
  void *context;
  motion_status_t (*set_direction)(void *context, motor_dir_t dir);
  motion_status_t (*set_speed)(void *context, uint8_t speed);
  motion_status_t (*stop)(void *context);
} motor_handle_t;

/**
 * @brief Set rotation direction for a motor handle.
 *
 * @param motor Pointer to motor_handle_t instance.
 * @param dir Desired motor direction (STOP, FORWARD, BACKWARD).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
static inline motion_status_t motor_set_direction(const motor_handle_t *motor, motor_dir_t dir) {
  if (motor == (void *)0 || motor->set_direction == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return motor->set_direction(motor->context, dir);
}

/**
 * @brief Set PWM speed for a motor handle.
 *
 * @param motor Pointer to motor_handle_t instance.
 * @param speed 8-bit duty cycle (0 = 0% OFF, 255 = 100% full speed).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
static inline motion_status_t motor_set_speed(const motor_handle_t *motor, uint8_t speed) {
  if (motor == (void *)0 || motor->set_speed == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return motor->set_speed(motor->context, speed);
}

/**
 * @brief Immediately stop a motor handle.
 *
 * @note Invokes motor->stop if implemented; falls back to motor_set_direction(STOP).
 *
 * @param motor Pointer to motor_handle_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG or error.
 */
static inline motion_status_t motor_stop(const motor_handle_t *motor) {
  if (motor == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  if (motor->stop != (void *)0) {
    return motor->stop(motor->context);
  }

  if (motor->set_direction != (void *)0) {
    return motor->set_direction(motor->context, MOTOR_DIR_STOP);
  }

  return MOTION_STATUS_ERROR;
}

#ifdef __cplusplus
}
#endif
