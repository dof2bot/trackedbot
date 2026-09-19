/**
 * @file l293d_shield.h
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

#include "imotor.h"
#include "motion_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Channels for L293D shield.
 */
typedef enum {
  L293D_CHANNEL_1 = 1,
  L293D_CHANNEL_2 = 2,
  L293D_CHANNEL_3 = 3,
  L293D_CHANNEL_4 = 4
} l293d_channel_t;

/**
 * @brief Initialize Timer0 for Fast PWM and reset motor shield hardware.
 *
 * Configures PD6 (OC0A) and PD5 (OC0B) as outputs, sets TCCR0A/TCCR0B
 * registers for non-inverting Fast PWM at ~976 Hz, sets initial duty
 * cycles to 0%, and resets all direction bits via the 74HC595 shift register.
 * With a frequency of ~1 kHz, the L293D H-bridge has minimal switching losses,
 * and the motors provide strong torque and smooth rotation.
 *
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t l293d_init(void);

/**
 * @brief Set the PWM duty cycle for an active motor channel.
 *
 * Updates OCR0A (Channel 3 / Left Track) or OCR0B (Channel 4 / Right Track).
 *
 * @param ch Motor channel identifier.
 * @param speed 8-bit duty cycle (0 = 0% OFF, 128 = 50%, 255 = 100% full speed).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on unsupported channel.
 */
motion_status_t l293d_set_speed(l293d_channel_t ch, uint8_t speed);

/**
 * @brief Set H-bridge direction bits for a motor channel via 74HC595.
 *
 * @param ch Motor channel (1 - 4).
 * @param dir Motor direction (MOTOR_DIR_STOP, MOTOR_DIR_FORWARD, MOTOR_DIR_BACKWARD).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid parameter.
 */
motion_status_t l293d_set_direction(l293d_channel_t ch, motor_dir_t dir);

/**
 * @brief Immediately stop an individual motor channel.
 *
 * Clears the PWM duty cycle to 0 and clears both H-bridge direction bits.
 *
 * @param ch Motor channel (1 - 4).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid channel.
 */
motion_status_t l293d_stop(l293d_channel_t ch);

/**
 * @brief Factory method creating an abstract motor handle for Dependency Injection.
 *
 * Binds the low-level L293D hardware channel to the abstract imotor.h interface.
 * Supports mechanical track inversion flag without altering core kinematics.
 *
 * @param ch Motor channel on the L293D shield.
 * @param inverted true if motor polarity is physically inverted.
 * @return motor_handle_t Abstract motor handle.
 */
motor_handle_t l293d_get_motor_handle(l293d_channel_t ch, bool inverted);

#ifdef __cplusplus
}
#endif
