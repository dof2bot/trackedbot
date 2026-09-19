/**
 * @file tracked_motion.c
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
 * Implements dual-track kinematics, speed setpoint clamping, trapezoidal slew rate
 * limiting, and direction sequencing for the Adafruit L293D dual-H-bridge shield.
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

#include "tracked_motion.h"
#include "progmem_compat.h"

enum {
  /** Default speed slew rate: 15 PWM units per update cycle (~170ms ramp from 0 to 255 at 100 Hz). */
  TRACKED_MOTION_SLEW_RATE_DEFAULT = 15
};

/**
 * @brief Clamp input speed within [TRACKED_MOTION_SPEED_MIN, TRACKED_MOTION_SPEED_MAX].
 *
 * @param speed Raw speed value.
 * @return uint8_t Clamped speed value (60 to 255).
 */
static uint8_t clamp_speed(uint8_t speed) {
  if (speed < (uint8_t)TRACKED_MOTION_SPEED_MIN) {
    return (uint8_t)TRACKED_MOTION_SPEED_MIN;
  }
  if (speed > (uint8_t)TRACKED_MOTION_SPEED_MAX) {
    return (uint8_t)TRACKED_MOTION_SPEED_MAX;
  }

  return speed;
}

/**
 * @brief Apply differential speeds to left and right motors based on active command.
 *
 * Fetches current ramped speed from the slew rate limiter and applies it to
 * left and right tracks according to the skid-steering truth table.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, error code on driver failure.
 */
static motion_status_t tracked_motion_apply_speeds(tracked_motion_t *tm) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  uint8_t cur_speed = slew_limiter_get_current(&tm->speed_limiter);
  motion_status_t status = MOTION_STATUS_OK;

  switch (tm->current_motion) {
  case MOTION_FORWARD:
  case MOTION_BACKWARD:
  case MOTION_SPIN_LEFT:
  case MOTION_SPIN_RIGHT:
    status = motor_set_speed(&tm->left_motor, cur_speed);
    if (status == MOTION_STATUS_OK) {
      status = motor_set_speed(&tm->right_motor, cur_speed);
    }
    break;

  case MOTION_TURN_LEFT:
    /* Skid steering: left track stop, right track at current ramped speed */
    status = motor_set_speed(&tm->left_motor, 0);
    if (status == MOTION_STATUS_OK) {
      status = motor_set_speed(&tm->right_motor, cur_speed);
    }
    break;

  case MOTION_TURN_RIGHT:
    /* Skid steering: left track at current ramped speed, right track stop */
    status = motor_set_speed(&tm->left_motor, cur_speed);
    if (status == MOTION_STATUS_OK) {
      status = motor_set_speed(&tm->right_motor, 0);
    }
    break;

  case MOTION_STOP:
  default:
    status = motor_set_speed(&tm->left_motor, 0);
    if (status == MOTION_STATUS_OK) {
      status = motor_set_speed(&tm->right_motor, 0);
    }
    break;
  }

  return status;
}

/**
 * @brief Initialize tracked mobile platform kinematics with motor handles.
 *
 * Configures left and right track motor abstractions, clamps initial speed,
 * initializes the slew rate limiter, and forces the platform into stopped idle state.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param left Left track motor handle (imotor.h).
 * @param right Right track motor handle (imotor.h).
 * @param initial_speed Initial target speed setpoint (60 - 255).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t
tracked_motion_init(tracked_motion_t *tm, motor_handle_t left, motor_handle_t right, uint8_t initial_speed) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  tm->left_motor = left;
  tm->right_motor = right;
  tm->target_speed = clamp_speed(initial_speed);
  tm->current_motion = MOTION_STOP;
  tm->state = MOTION_STATE_IDLE;

  (void)slew_limiter_init(&tm->speed_limiter, 0, (uint8_t)TRACKED_MOTION_SLEW_RATE_DEFAULT);

  return tracked_motion_stop(tm);
}

/**
 * @brief Execute a motion command on the tracked chassis (skid-steering / differential).
 *
 * Sets motor direction pins, programs the target speed setpoint into the slew limiter,
 * transitions platform state to MOVING, and applies initial ramped speeds.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param cmd Motion command (FORWARD, BACKWARD, TURN_LEFT, SPIN_LEFT, etc.).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid arg or cmd.
 */
motion_status_t tracked_motion_set_command(tracked_motion_t *tm, motion_cmd_t cmd) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  tm->current_motion = cmd;

  switch (cmd) {
  case MOTION_FORWARD:
    motor_set_direction(&tm->left_motor, MOTOR_DIR_FORWARD);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_FORWARD);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_BACKWARD:
    motor_set_direction(&tm->left_motor, MOTOR_DIR_BACKWARD);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_BACKWARD);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_TURN_LEFT:
    /* Skid steering: left track stop, right track forward */
    motor_set_direction(&tm->left_motor, MOTOR_DIR_STOP);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_FORWARD);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_TURN_RIGHT:
    /* Skid steering: left track forward, right track stop */
    motor_set_direction(&tm->left_motor, MOTOR_DIR_FORWARD);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_STOP);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_SPIN_LEFT:
    /* Turn in place: left backward, right forward */
    motor_set_direction(&tm->left_motor, MOTOR_DIR_BACKWARD);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_FORWARD);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_SPIN_RIGHT:
    /* Turn in place: left forward, right backward */
    motor_set_direction(&tm->left_motor, MOTOR_DIR_FORWARD);
    motor_set_direction(&tm->right_motor, MOTOR_DIR_BACKWARD);
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
    tm->state = MOTION_STATE_MOVING;
    tracked_motion_apply_speeds(tm);
    return MOTION_STATUS_OK;

  case MOTION_STOP:
    return tracked_motion_stop(tm);

  default:
    return MOTION_STATUS_INVALID_ARG;
  }
}

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
motion_status_t tracked_motion_set_speed(tracked_motion_t *tm, uint8_t speed) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  tm->target_speed = clamp_speed(speed);

  if (tm->state == MOTION_STATE_MOVING && tm->current_motion != MOTION_STOP) {
    (void)slew_limiter_set_target(&tm->speed_limiter, tm->target_speed);
  }

  return MOTION_STATUS_OK;
}

/**
 * @brief Periodic kinematics update routine (steps slew rate limiter and updates motor PWM).
 *
 * Advances the trapezoidal speed profile by one step toward target_speed and writes
 * the new PWM values to the motor drivers. Must be called periodically from the main loop.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @param delta_ms Elapsed milliseconds since last update.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t tracked_motion_update(tracked_motion_t *tm, uint16_t delta_ms) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }
  (void)delta_ms;

  if (tm->state == MOTION_STATE_MOVING) {
    (void)slew_limiter_update(&tm->speed_limiter);

    return tracked_motion_apply_speeds(tm);
  }

  return MOTION_STATUS_OK;
}

/**
 * @brief Immediate stop of all motion, reset slew limiter and motors, transition to IDLE.
 *
 * Sets PWM to 0, puts H-bridge in active dynamic braking (STOP), clears slew limiter,
 * and sets state to MOTION_STATE_IDLE.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if tm is NULL.
 */
motion_status_t tracked_motion_stop(tracked_motion_t *tm) {
  if (tm == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  motion_status_t s1 = motor_stop(&tm->left_motor);
  motion_status_t s2 = motor_stop(&tm->right_motor);
  (void)slew_limiter_reset(&tm->speed_limiter, 0);

  tm->current_motion = MOTION_STOP;
  tm->state = MOTION_STATE_IDLE;

  if (s1 != MOTION_STATUS_OK) {
    return s1;
  }

  return s2;
}

/**
 * @brief Get configured target speed setpoint.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return uint8_t Target speed, or 0 if tm is NULL.
 */
uint8_t tracked_motion_get_speed(const tracked_motion_t *tm) {
  if (tm == (void *)0) {
    return 0;
  }

  return tm->target_speed;
}

/**
 * @brief Get current operational state.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_state_t Current state (IDLE, MOVING, FAILSAFE), or MOTION_STATE_IDLE if tm is NULL.
 */
motion_state_t tracked_motion_get_state(const tracked_motion_t *tm) {
  if (tm == (void *)0) {
    return MOTION_STATE_IDLE;
  }

  return tm->state;
}

/**
 * @brief Get current active motion command.
 *
 * @param tm Pointer to tracked_motion_t instance.
 * @return motion_cmd_t Active command enum, or MOTION_STOP if tm is NULL.
 */
motion_cmd_t tracked_motion_get_command(const tracked_motion_t *tm) {
  if (tm == (void *)0) {
    return MOTION_STOP;
  }

  return tm->current_motion;
}

/** Motion command string lookup table stored in Flash memory (PROGMEM). */
static const char S_MOTION_CMD_STRINGS[][11] PROGMEM = {
    "STOP",
    "FORWARD",
    "BACKWARD",
    "TURN_LEFT",
    "TURN_RIGHT",
    "SPIN_LEFT",
    "SPIN_RIGHT",
};

/** Motion state string lookup table stored in Flash memory (PROGMEM). */
static const char S_MOTION_STATE_STRINGS[][9] PROGMEM = {
    "IDLE",
    "MOVING",
    "FAILSAFE",
};

/**
 * @brief Convert motion command enum to human-readable ASCII string (stored in Flash).
 *
 * @param cmd Motion command enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "FORWARD", "STOP", etc.).
 */
const char *tracked_motion_cmd_to_str(motion_cmd_t cmd) {
  if ((uint8_t)cmd < (sizeof(S_MOTION_CMD_STRINGS) / sizeof(S_MOTION_CMD_STRINGS[0]))) {
    return S_MOTION_CMD_STRINGS[cmd];
  }

  return PSTR("UNKNOWN");
}

/**
 * @brief Convert motion state enum to human-readable ASCII string (stored in Flash).
 *
 * @param state Motion state enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "IDLE", "MOVING", "FAILSAFE").
 */
const char *tracked_motion_state_to_str(motion_state_t state) {
  if ((uint8_t)state < (sizeof(S_MOTION_STATE_STRINGS) / sizeof(S_MOTION_STATE_STRINGS[0]))) {
    return S_MOTION_STATE_STRINGS[state];
  }

  return PSTR("UNKNOWN");
}
