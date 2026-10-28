/**
 * @file l293d_shield.c
 * Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
 *
 * trackedbot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * trackedbot is distributed in the hope that it will be useful, but
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
 * Each DC track motor is allocated to a separate physical L293D IC:
 *  - IC2: Drives Motor 3 (M3) dedicated to the Left Track.
 *  - IC1: Drives Motor 1 (M1) dedicated to the Right Track.
 * This 1-chip-per-motor architecture eliminates thermal choking, avoids
 * current sharing on a single chip, and provides maximum torque and power.
 *
 * Fast PWM Hardware Speed Control (ATmega328P):
 *  - Left Track (M3): Timer0 Fast PWM on OC0A (PD6 / Arduino Pin 6).
 *  - Right Track (M1): Timer2 Fast PWM on OC2A (PB3 / Arduino Pin 11).
 *  - Both Timers operate in Fast PWM Mode 3: 8-bit counter, TOP = 0xFF (255).
 *  - Clock Prescalers:
 *      Timer0 (N = 64): CS01=1, CS00=1 -> f_PWM = 16 MHz / (64 * 256) ~ 976.56 Hz
 *      Timer2 (N = 64): CS22=1         -> f_PWM = 16 MHz / (64 * 256) ~ 976.56 Hz
 *  - Both channels run at the exact same ~976 Hz frequency and 8-bit resolution.
 *
 * Fast PWM Waveform Timing Diagram:
 *
 * TCNT0 / TCNT2 (Counters)
 *     ^
 * 255 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - TOP
 *     |            /|            /|            /|            /|
 * OCR | - - - - - / | - - - - - / | - - - - - / | - - - - - / | - - - - - OCR0A / OCR2A
 *     |          /  |          /  |          /  |          /  |
 *   0 +---------+---+---------+---+---------+---+---------+---+---------> Time
 * Output Pins: PD6 (OC0A, M3 Left) / PB3 (OC2A, M1 Right)
 *   1 |=========|   |=========|   |=========|   |=========|   |
 *   0 +---------+---+---------+---+---------+---+---------+---+--------->
 *     |<--Duty->|   |<--Duty->|   |<--Duty->|   |<--Duty->|   |
 *     |<------ Period ~1.024ms (976 Hz) ------->|
 *
 * Hardware Interconnection Block Diagram:
 *
 *      ATmega328P                         L293D (IC2 - Left Track)
 *    +-------------+                    +---------------+
 *    | PD6 (OC0A)  |---[PWM Speed M3]-->| 1,2EN  (Pin 1)|
 *    | 74HC595 Q5  |---[Dir A (M3_A)]-->| 1A     (Pin 2)|---> M3 Terminals (Left Track)
 *    | 74HC595 Q7  |---[Dir B (M3_B)]-->| 2A     (Pin 7)|
 *    |             |                    +---------------+
 *    |             |
 *    |             |                      L293D (IC1 - Right Track)
 *    |             |                    +---------------+
 *    | PB3 (OC2A)  |---[PWM Speed M1]-->| 1,2EN  (Pin 1)|
 *    | 74HC595 Q2  |---[Dir A (M1_A)]-->| 1A/3A         |---> M1 Terminals (Right Track)
 *    | 74HC595 Q3  |---[Dir B (M1_B)]-->| 2A/4A         |
 *    +-------------+                    +---------------+
 *
 * L293D H-Bridge Truth Table (M3 Left Track / M1 Right Track):
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

/* Hardware PWM Output Pin Bit Positions on AVR Ports (PD6 / PB3). */
enum {
  MOTOR3_PWM_BIT = PORTD6, /**< Arduino Pin 6 -> PD6 / OC0A (Timer0 PWM output). */
  MOTOR1_PWM_BIT = PORTB3  /**< Arduino Pin 11 -> PB3 / OC2A (Timer2 PWM output). */
};

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

/* Maximum number of active motor adapter contexts allocated in SRAM. */
enum {
  L293D_MAX_MOTOR_CONTEXTS = 2 /**< Number of concurrently instantiated motor handles (Left & Right). */
};

/**
 * @brief Stores motor context for each active motor channel.
 */
static l293d_motor_ctx_t s_motor_contexts[L293D_MAX_MOTOR_CONTEXTS];

/**
 * @brief Initialize Timer0 and Timer2 for Fast PWM and reset motor shield hardware.
 *
 * Configures PD6 (OC0A, M3 / Left Track) and PB3 (OC2A, M1 / Right Track) as outputs,
 * sets TCCR0A/TCCR0B and TCCR2A/TCCR2B registers for non-inverting Fast PWM at ~976 Hz,
 * sets initial duty cycles to 0%, and resets all direction bits via the 74HC595 shift register.
 * With a frequency of ~1 kHz, the L293D H-bridge has minimal switching losses,
 * and the motors provide strong torque and smooth rotation. Each motor operates on its
 * own dedicated L293D driver IC (M3 on IC2, M1 on IC1) to maximize current delivery and
 * distribute heat dissipation evenly.
 *
 * @return MOTION_STATUS_OK on success.
 */
motion_status_t l293d_init(void) {
  /* Initialize 74HC595 shift register (set 0 and enable outputs). */
  (void)shift_reg_init();

  /* Configure PWM pins as outputs on Port D (M3 / PD6) and Port B (M1 / PB3). */
  DDRD |= (uint8_t)(1 << MOTOR3_PWM_BIT);
  DDRB |= (uint8_t)(1 << MOTOR1_PWM_BIT);

  /* Configure Timer0 for Fast PWM (mode 3, non-inverting on OC0A for M3). */
  TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);

  /* Prescaler = 64: 16MHz / (64 * 256) ~ 976.56 Hz. */
  TCCR0B = (1 << CS01) | (1 << CS00);

  /* Configure Timer2 for Fast PWM (mode 3, non-inverting on OC2A for M1). */
  TCCR2A = (1 << COM2A1) | (1 << WGM21) | (1 << WGM20);

  /* Prescaler = 64: 16MHz / (64 * 256) ~ 976.56 Hz. */
  TCCR2B = (1 << CS22);

  /* Start speed set to 0% duty cycle on active channels. */
  OCR0A = 0;
  OCR2A = 0;

  /* Stop all H-bridges. */
  (void)l293d_stop(L293D_CHANNEL_1);
  (void)l293d_set_direction(L293D_CHANNEL_2, MOTOR_DIR_STOP);
  (void)l293d_stop(L293D_CHANNEL_3);
  (void)l293d_set_direction(L293D_CHANNEL_4, MOTOR_DIR_STOP);

  return MOTION_STATUS_OK;
}

/**
 * @brief Set the PWM duty cycle for an active motor channel.
 *
 * Updates OCR0A (Channel 3 / Left Track) or OCR2A (Channel 1 / Right Track).
 *
 * @param ch Motor channel identifier.
 * @param speed 8-bit duty cycle (0 = 0% OFF, 128 = 50%, 255 = 100% full speed).
 * @return MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on unsupported channel.
 */
motion_status_t l293d_set_speed(l293d_channel_t ch, uint8_t speed) {
  switch (ch) {
  case L293D_CHANNEL_1:
    OCR2A = speed;
    return MOTION_STATUS_OK;

  case L293D_CHANNEL_3:
    OCR0A = speed;
    return MOTION_STATUS_OK;

  case L293D_CHANNEL_2:
  case L293D_CHANNEL_4:
  default:
    /* Channels 2 and 4 are unused on the tracked platform (M1 Right Track, M3 Left Track) */
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
  uint8_t index = 0;

  for (uint8_t i = 0; i < (uint8_t)L293D_MAX_MOTOR_CONTEXTS; ++i) {
    if (s_motor_contexts[i].channel == ch || s_motor_contexts[i].channel == (l293d_channel_t)0) {
      index = i;
      break;
    }
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
