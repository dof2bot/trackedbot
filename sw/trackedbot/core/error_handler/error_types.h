/**
 * @file error_types.h
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
 * @brief System error diagnostic codes, operating modes, and safety reactions.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System operating modes indicating health and autonomy level.
 */
typedef enum {
  SYSTEM_MODE_NORMAL = 0, /**< All subsystems nominal; full motion and telemetry permitted. */
  SYSTEM_MODE_DEGRADED,   /**< Fault latched; motors locked or comm restricted for safety. */
  SYSTEM_MODE_PANIC       /**< Critical hardware fault; requires system restart or safe halt. */
} system_mode_t;

/**
 * @brief System error diagnostic codes reported across firmware subsystems.
 */
typedef enum {
  ERR_CODE_NONE = 0, /**< No error active. */
  /* Initialization errors */
  ERR_CODE_INIT_UART,     /**< Failed to initialize UART peripheral or baud rate. */
  ERR_CODE_INIT_MOTORS,   /**< Failed to initialize motor driver / shift register. */
  ERR_CODE_INIT_TIMER,    /**< Failed to initialize system tick timer. */
  ERR_CODE_INIT_PARSER,   /**< Failed to initialize command parser. */
  ERR_CODE_INIT_FAILSAFE, /**< Failed to initialize failsafe watchdog. */
  /* Runtime errors */
  ERR_CODE_COMM_RX_OVERFLOW,   /**< UART RX ring buffer overflow (incoming byte dropped). */
  ERR_CODE_COMM_TX_OVERFLOW,   /**< UART TX ring buffer overflow (outgoing byte dropped). */
  ERR_CODE_CMD_SYNTAX_ERROR,   /**< Malformed command frame, invalid length or argument. */
  ERR_CODE_CMD_UNKNOWN,        /**< Unrecognized command message ID received. */
  ERR_CODE_FAILSAFE_TRIGGERED, /**< Communication heartbeat timeout triggered emergency stop. */
  ERR_CODE_HARDWARE_FAULT,     /**< Fatal hardware condition or internal assertion failure. */
  ERR_CODE_COMM_CRC            /**< Frame CRC-8 checksum mismatch. */
} error_code_t;

/**
 * @brief Safety reaction policy dispatched on error occurrence.
 */
typedef enum {
  REACTION_NONE = 0,                   /**< No action required. */
  REACTION_LOG_ONLY,                   /**< Increment counter and latch error code without altering mode. */
  REACTION_STOP_MOTION,                /**< Stop tracked motion immediately. */
  REACTION_ENTER_DEGRADED_LOCK_MOTION, /**< Transition to DEGRADED mode and inhibit all motion commands. */
  REACTION_ENTER_DEGRADED_LOCK_COMM,   /**< Transition to DEGRADED mode and restrict communication. */
  REACTION_SYSTEM_RESET                /**< Transition to PANIC mode and trigger hardware reset. */
} safety_reaction_t;

#ifdef __cplusplus
}
#endif
