/**
 * @file l293d_shield.c
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
 * @brief Dual L293D Motor Shield hardware adapter for tracked chassis.
 *
 * Controls DC motors via two L293D quadruple half-H drivers and a 74HC595
 * shift register for direction selection.
 *
 * Timer0 Fast PWM Configuration (Speed Control):
 *  - Channels M3 & M4 use Timer0 on ATmega328P.
 *  - Fast PWM Mode 3: 8-bit counter, TOP = 0xFF (255).
 *  - Non-inverting mode on OC0A (M3) and OC0B (M4).
 *  - Clock Prescaler = 64:
 *      f_PWM = f_CPU / (N * 256) = 16,000,000 / (64 * 256) ~ 976.56 Hz
 *
 * Fast PWM Waveform Timing Diagram:
 *
 * TCNT0 (Counter)
 *     ^
 * 255 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TOP
 *     |            /|            /|            /|            /|
 * OCR | - - - - - / | - - - - - / | - - - - - / | - - - - - / | - - - - - OCR0A/B
 *     |          /  |          /  |          /  |          /  |
 *   0 +---------+---+---------+---+---------+---+---------+---+---------> Time
 * OC0x Pin (PD6 / PD5)
 *   1 |=========|   |=========|   |=========|   |=========|   |
 *   0 +---------+---+---------+---+---------+---+---------+---+--------->
 *     |<--Duty->|   |<--Duty->|   |<--Duty->|   |<--Duty->|   |
 *     |<------ Period ~1.024ms (976 Hz) ------->|
 *
 * Hardware Interconnection Block Diagram:
 *
 *      ATmega328P                         L293D (IC2)
 *    +-------------+                    +---------------+
 *    | PD6 (OC0A)  |---[PWM Speed M3]-->| 1,2EN  (Pin 1)|
 *    | 74HC595 Q5  |---[Dir A (M3_A)]-->| 1A     (Pin 2)|---> M3 Output 1 (Left Track)
 *    | 74HC595 Q7  |---[Dir B (M3_B)]-->| 2A     (Pin 7)|---> M3 Output 2
 *    |             |                    |               |
 *    | PD5 (OC0B)  |---[PWM Speed M4]-->| 3,4EN  (Pin 9)|
 *    | 74HC595 Q0  |---[Dir A (M4_A)]-->| 3A    (Pin 10)|---> M4 Output 1 (Right Track)
 *    | 74HC595 Q6  |---[Dir B (M4_B)]-->| 4A    (Pin 15)|---> M4 Output 2
 *    +-------------+                    +---------------+
 *
 * L293D H-Bridge Truth Table:
 *  +--------------+--------+--------+--------------------------+
 *  | PWM (EN)     | Dir A  | Dir B  | Motor State              |
 *  +--------------+--------+--------+--------------------------+
 *  | 0            | X      | X      | Off / Coast              |
 *  | PWM (0..255) | 1      | 0      | Forward Drive (Active)   |
 *  | PWM (0..255) | 0      | 1      | Reverse Drive (Active)   |
 *  | PWM (0..255) | 0      | 0      | Dynamic Brake / Stop     |
 *  +--------------+--------+--------+--------------------------+
 */

#include "l293d_shield.h"
#include "shift_reg.h"
#include <avr/io.h>

/* M3 (left motor): Arduino Pin 6 -> PD6 / OC0A (Timer0 comparator A PWM output). */
static const uint8_t MOTOR3_PWM_BIT = PORTD6;

/* M4 (right motor): Arduino Pin 5 -> PD5 / OC0B (Timer0 comparator B PWM output). */
static const uint8_t MOTOR4_PWM_BIT = PORTD5;

/* Localized 74HC595 Output Bit Positions for L293D H-Bridges. */
enum {
  SHIFT_BIT_MOTOR4_A = 0,
  SHIFT_BIT_MOTOR2_A = 1,
  SHIFT_BIT_MOTOR1_A = 2,
  SHIFT_BIT_MOTOR1_B = 3,
  SHIFT_BIT_MOTOR2_B = 4,
  SHIFT_BIT_MOTOR3_A = 5,
  SHIFT_BIT_MOTOR4_B = 6,
  SHIFT_BIT_MOTOR3_B = 7
};

/**
 * @brief Context structure for an individual motor channel adapter instance.
 *
 * @param channel L293D channel (1 - 4).
 * @param inverted Invert direction (true) or not (false).
 */
typedef struct {
  l293d_channel_t channel;
  bool inverted;
} l293d_motor_ctx_t;

/**
 * @brief Stores motor context for each motor channel.
 */
static l293d_motor_ctx_t s_motor_contexts[4];

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
motion_status_t l293d_init(void) {
  /* Initialize 74HC595 shift register (set 0 and enable outputs). */
  (void)shift_reg_init();

  /* Configure PWM pins as outputs on Port D. */
  DDRD |= (uint8_t)((1 << MOTOR3_PWM_BIT) | (1 << MOTOR4_PWM_BIT));

  /* Configure Timer0 for Fast PWM (mode 3, non-inverting on OC0A and OC0B). */
  TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);

  /* Prescaler = 64: 16MHz / (64 * 256) ~ 976 Hz. */
  TCCR0B = (1 << CS01) | (1 << CS00);

  /* Start speed set to 0% duty cycle. */
  OCR0A = 0;
  OCR0B = 0;

  /* Stop all H-bridges. */
  (void)l293d_set_direction(L293D_CHANNEL_1, MOTOR_DIR_STOP);
  (void)l293d_set_direction(L293D_CHANNEL_2, MOTOR_DIR_STOP);
  (void)l293d_stop(L293D_CHANNEL_3);
  (void)l293d_stop(L293D_CHANNEL_4);

  return MOTION_STATUS_OK;
}

/**
 * @brief Set the PWM duty cycle for an active motor channel.
 *
 * Updates OCR0A (Channel 3 / Left Track) or OCR0B (Channel 4 / Right Track).
 *
 * @param ch Motor channel identifier.
 * @param speed 8-bit duty cycle (0 = 0% OFF, 128 = 50%, 255 = 100% full speed).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on unsupported channel.
 */
motion_status_t l293d_set_speed(l293d_channel_t ch, uint8_t speed) {
  switch (ch) {
  case L293D_CHANNEL_3:
    OCR0A = speed;
    return MOTION_STATUS_OK;

  case L293D_CHANNEL_4:
    OCR0B = speed;
    return MOTION_STATUS_OK;

  case L293D_CHANNEL_1:
  case L293D_CHANNEL_2:
  default:
    /* Channels 1 and 2 use Timer2 if needed; unsupported on Timer0 */
    return MOTION_STATUS_INVALID_ARG;
  }
}

/**
 * @brief Set H-bridge direction bits for a motor channel via 74HC595.
 *
 * @param ch Motor channel (1 - 4).
 * @param dir Motor direction (MOTOR_DIR_STOP, MOTOR_DIR_FORWARD, MOTOR_DIR_BACKWARD).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid parameter.
 */
motion_status_t l293d_set_direction(l293d_channel_t ch, motor_dir_t dir) {
  uint8_t bit_a;
  uint8_t bit_b;

  switch (ch) {
  case L293D_CHANNEL_1:
    bit_a = SHIFT_BIT_MOTOR1_A;
    bit_b = SHIFT_BIT_MOTOR1_B;
    break;

  case L293D_CHANNEL_2:
    bit_a = SHIFT_BIT_MOTOR2_A;
    bit_b = SHIFT_BIT_MOTOR2_B;
    break;

  case L293D_CHANNEL_3:
    bit_a = SHIFT_BIT_MOTOR3_A;
    bit_b = SHIFT_BIT_MOTOR3_B;
    break;

  case L293D_CHANNEL_4:
    bit_a = SHIFT_BIT_MOTOR4_A;
    bit_b = SHIFT_BIT_MOTOR4_B;
    break;

  default:
    return MOTION_STATUS_INVALID_ARG;
  }

  switch (dir) {
  case MOTOR_DIR_FORWARD:
    (void)shift_reg_set_bit(bit_a, 1);
    (void)shift_reg_set_bit(bit_b, 0);
    break;

  case MOTOR_DIR_BACKWARD:
    (void)shift_reg_set_bit(bit_a, 0);
    (void)shift_reg_set_bit(bit_b, 1);
    break;

  case MOTOR_DIR_STOP:
    (void)shift_reg_set_bit(bit_a, 0);
    (void)shift_reg_set_bit(bit_b, 0);
    break;

  default:
    return MOTION_STATUS_INVALID_ARG;
  }

  (void)shift_reg_update();

  return MOTION_STATUS_OK;
}

/**
 * @brief Immediately stop an individual motor channel.
 *
 * Clears the PWM duty cycle to 0 and clears both H-bridge direction bits.
 *
 * @param ch Motor channel (1 - 4).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid channel.
 */
motion_status_t l293d_stop(l293d_channel_t ch) {
  motion_status_t s_speed = l293d_set_speed(ch, 0);
  motion_status_t s_dir = l293d_set_direction(ch, MOTOR_DIR_STOP);

  if (s_speed != MOTION_STATUS_OK) {
    return s_speed;
  }

  return s_dir;
}

/**
 * @brief Adaptor function for setting motor direction.
 *
 * Implements the imotor.h contract for motor direction control.
 *
 * @param context Context pointer (l293d_motor_ctx_t *).
 * @param dir Motor direction.
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid argument.
 */
static motion_status_t l293d_adapter_set_direction(void *context, motor_dir_t dir) {
  l293d_motor_ctx_t *ctx = (l293d_motor_ctx_t *)context;

  if (ctx == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  motor_dir_t effective_dir = dir;

  if (ctx->inverted) {
    if (dir == MOTOR_DIR_FORWARD) {
      effective_dir = MOTOR_DIR_BACKWARD;
    } else if (dir == MOTOR_DIR_BACKWARD) {
      effective_dir = MOTOR_DIR_FORWARD;
    }
  }

  return l293d_set_direction(ctx->channel, effective_dir);
}

/**
 * @brief Adaptor function for setting motor speed.
 *
 * Implements the imotor.h contract for motor speed control.
 *
 * @param context Context pointer (l293d_motor_ctx_t *).
 * @param speed Motor speed (0 - 255).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid argument.
 */
static motion_status_t l293d_adapter_set_speed(void *context, uint8_t speed) {
  l293d_motor_ctx_t *ctx = (l293d_motor_ctx_t *)context;

  if (ctx == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return l293d_set_speed(ctx->channel, speed);
}

/**
 * @brief Adaptor function for stopping a motor.
 *
 * Implements the imotor.h contract for stopping a motor.
 *
 * @param context Context pointer (l293d_motor_ctx_t *).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on invalid argument.
 */
static motion_status_t l293d_adapter_stop(void *context) {
  l293d_motor_ctx_t *ctx = (l293d_motor_ctx_t *)context;

  if (ctx == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return l293d_stop(ctx->channel);
}

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
motor_handle_t l293d_get_motor_handle(l293d_channel_t ch, bool inverted) {
  uint8_t index = (uint8_t)(ch - 1);

  if (index > 3) {
    index = 0;
  }

  s_motor_contexts[index].channel = ch;
  s_motor_contexts[index].inverted = inverted;

  motor_handle_t handle;
  handle.context = (void *)&s_motor_contexts[index];
  handle.set_direction = l293d_adapter_set_direction;
  handle.set_speed = l293d_adapter_set_speed;
  handle.stop = l293d_adapter_stop;

  return handle;
}
