/**
 * @file error_handler.h
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
 * @brief System error handling, health monitoring, and safety reaction service.
 *
 * Tracks operating health modes (NORMAL, DEGRADED, PANIC), aggregates error counts,
 * latches latest diagnostic codes, and evaluates safety interlocks (e.g. motion lock).
 *
 * System Mode State Machine:
 *
 *     +-----------------------+
 *     |  SYSTEM_MODE_NORMAL   |<-------------------+
 *     +-----------+-----------+                    |
 *                 |                                | error_handler_clear()
 *                 | Init Error / Safety Violation  |
 *                 v                                |
 *     +-----------------------+                    |
 *     | SYSTEM_MODE_DEGRADED  |--------------------+
 *     +-----------+-----------+
 *                 |
 *                 | Hardware Fault (Panic)
 *                 v
 *     +-----------------------+
 *     |   SYSTEM_MODE_PANIC   |----> Hardware WDT / Software Reset
 *     +-----------------------+
 *
 * Safety Policy Matrix:
 *  +-----------------------------+-----------------------+--------------------------------------+
 *  | Error Code                  | Transitioned Mode     | Safety Reaction Dispatched           |
 *  +-----------------------------+-----------------------+--------------------------------------+
 *  | ERR_CODE_INIT_UART          | SYSTEM_MODE_DEGRADED  | REACTION_ENTER_DEGRADED_LOCK_COMM    |
 *  | ERR_CODE_INIT_MOTORS        | SYSTEM_MODE_DEGRADED  | REACTION_ENTER_DEGRADED_LOCK_MOTION  |
 *  | ERR_CODE_INIT_TIMER/PARSER  | SYSTEM_MODE_DEGRADED  | REACTION_ENTER_DEGRADED_LOCK_MOTION  |
 *  | ERR_CODE_FAILSAFE_TRIGGERED | (Retains Mode)        | REACTION_STOP_MOTION                 |
 *  | ERR_CODE_HARDWARE_FAULT     | SYSTEM_MODE_PANIC     | REACTION_SYSTEM_RESET                |
 *  | ERR_CODE_COMM_* / CMD_*     | (Retains Mode)        | REACTION_LOG_ONLY                    |
 *  +-----------------------------+-----------------------+--------------------------------------+
 */

#pragma once

#include "error_types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Error handler state tracking structure.
 */
typedef struct {
  system_mode_t current_mode; /**< Active system health operating mode (NORMAL, DEGRADED, PANIC). */
  error_code_t last_error;    /**< Most recently latched error diagnostic code. */
  uint16_t error_count;       /**< Cumulative count of all reported error occurrences. */
} error_handler_t;

/**
 * @brief Initialize error handler component in normal operating mode.
 *
 * Resets error count to 0, latched error to ERR_CODE_NONE, and mode to SYSTEM_MODE_NORMAL.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success, false if eh is NULL.
 */
bool error_handler_init(error_handler_t *eh);

/**
 * @brief Report a system error, update internal state and determine safety reaction.
 *
 * Increments cumulative error counter, latches diagnostic code, transitions mode if
 * severity demands it, and returns recommended immediate safety reaction.
 *
 * @param eh Pointer to error_handler_t instance.
 * @param code Error diagnostic code.
 * @return safety_reaction_t Recommended safety action according to safety policy matrix.
 */
safety_reaction_t error_handler_report(error_handler_t *eh, error_code_t code);

/**
 * @brief Get current system operational mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return system_mode_t Current mode (NORMAL, DEGRADED, PANIC), or SYSTEM_MODE_PANIC if eh is NULL.
 */
system_mode_t error_handler_get_mode(const error_handler_t *eh);

/**
 * @brief Get last recorded error code.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return error_code_t Last error code, or ERR_CODE_NONE if eh is NULL.
 */
error_code_t error_handler_get_last_error(const error_handler_t *eh);

/**
 * @brief Query if motor motion is permitted by the safety policy.
 *
 * Motion is only permitted when system mode is strictly SYSTEM_MODE_NORMAL.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true if motion is allowed, false in degraded or panic mode (or if eh is NULL).
 */
bool error_handler_is_motion_allowed(const error_handler_t *eh);

/**
 * @brief Get total recorded error count.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return uint16_t Total number of reported errors, or 0 if eh is NULL.
 */
uint16_t error_handler_get_error_count(const error_handler_t *eh);

/**
 * @brief Clear last recorded error and restore normal operating mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success, false if eh is NULL.
 */
bool error_handler_clear(error_handler_t *eh);

/**
 * @brief Convert system mode enum to human-readable ASCII string (stored in Flash).
 *
 * @param mode System mode enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "NORMAL", "DEGRADED", "PANIC").
 */
const char *error_handler_mode_to_str(system_mode_t mode);

/**
 * @brief Convert error code enum to human-readable ASCII string (stored in Flash).
 *
 * @param code Error code enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "NONE", "INIT_UART", etc.).
 */
const char *error_handler_code_to_str(error_code_t code);

#ifdef __cplusplus
}
#endif
