/**
 * @file error_handler.c
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

#include "error_handler.h"
#include "progmem_compat.h"

/**
 * @brief Initialize error handler component in normal operating mode.
 *
 * Resets error count to 0, latched error to ERR_CODE_NONE, and mode to SYSTEM_MODE_NORMAL.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success, false if eh is NULL.
 */
bool error_handler_init(error_handler_t *eh) {
  if (eh == (void *)0) {
    return false;
  }

  eh->current_mode = SYSTEM_MODE_NORMAL;
  eh->last_error = ERR_CODE_NONE;
  eh->error_count = 0;

  return true;
}

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
safety_reaction_t error_handler_report(error_handler_t *eh, error_code_t code) {
  if (eh == (void *)0) {
    return REACTION_NONE;
  }

  eh->last_error = code;
  eh->error_count++;

  switch (code) {
  case ERR_CODE_INIT_UART:
    eh->current_mode = SYSTEM_MODE_DEGRADED;
    return REACTION_ENTER_DEGRADED_LOCK_COMM;

  case ERR_CODE_INIT_MOTORS:
  case ERR_CODE_INIT_TIMER:
  case ERR_CODE_INIT_PARSER:
  case ERR_CODE_INIT_FAILSAFE:
    eh->current_mode = SYSTEM_MODE_DEGRADED;
    return REACTION_ENTER_DEGRADED_LOCK_MOTION;

  case ERR_CODE_FAILSAFE_TRIGGERED:
    return REACTION_STOP_MOTION;

  case ERR_CODE_HARDWARE_FAULT:
    eh->current_mode = SYSTEM_MODE_PANIC;
    return REACTION_SYSTEM_RESET;

  case ERR_CODE_COMM_RX_OVERFLOW:
  case ERR_CODE_COMM_TX_OVERFLOW:
  case ERR_CODE_CMD_SYNTAX_ERROR:
  case ERR_CODE_CMD_UNKNOWN:
  case ERR_CODE_COMM_CRC:
    return REACTION_LOG_ONLY;

  case ERR_CODE_NONE:
  default:
    return REACTION_NONE;
  }
}

/**
 * @brief Get current system operational mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return system_mode_t Current mode (NORMAL, DEGRADED, PANIC), or SYSTEM_MODE_PANIC if eh is NULL.
 */
system_mode_t error_handler_get_mode(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return SYSTEM_MODE_PANIC;
  }

  return eh->current_mode;
}

/**
 * @brief Get last recorded error code.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return error_code_t Last error code, or ERR_CODE_NONE if eh is NULL.
 */
error_code_t error_handler_get_last_error(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return ERR_CODE_NONE;
  }

  return eh->last_error;
}

/**
 * @brief Query if motor motion is permitted by the safety policy.
 *
 * Motion is only permitted when system mode is strictly SYSTEM_MODE_NORMAL.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true if motion is allowed, false in degraded or panic mode (or if eh is NULL).
 */
bool error_handler_is_motion_allowed(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return false;
  }

  return (eh->current_mode == SYSTEM_MODE_NORMAL);
}

/**
 * @brief Get total recorded error count.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return uint16_t Total number of reported errors, or 0 if eh is NULL.
 */
uint16_t error_handler_get_error_count(const error_handler_t *eh) {
  if (eh == (void *)0) {
    return 0;
  }

  return eh->error_count;
}

/**
 * @brief Clear last recorded error and restore normal operating mode.
 *
 * @param eh Pointer to error_handler_t instance.
 * @return bool true on success, false if eh is NULL.
 */
bool error_handler_clear(error_handler_t *eh) {
  if (eh == (void *)0) {
    return false;
  }

  eh->last_error = ERR_CODE_NONE;
  eh->current_mode = SYSTEM_MODE_NORMAL;

  return true;
}

/** System mode string lookup table stored in Flash memory (PROGMEM). */
static const char S_MODE_STRINGS[][9] PROGMEM = {
    "NORMAL",
    "DEGRADED",
    "PANIC",
};

/** Error diagnostic code string lookup table stored in Flash memory (PROGMEM). */
static const char S_ERROR_CODE_STRINGS[][14] PROGMEM = {
    "NONE",
    "INIT_UART",
    "INIT_MOTORS",
    "INIT_TIMER",
    "INIT_PARSER",
    "INIT_FAILSAFE",
    "RX_OVERFLOW",
    "TX_OVERFLOW",
    "CMD_SYNTAX",
    "CMD_UNKNOWN",
    "FAILSAFE_STOP",
    "HW_FAULT",
    "COMM_CRC",
};

/**
 * @brief Convert system mode enum to human-readable ASCII string (stored in Flash).
 *
 * @param mode System mode enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "NORMAL", "DEGRADED", "PANIC").
 */
const char *error_handler_mode_to_str(system_mode_t mode) {
  if ((uint8_t)mode < (sizeof(S_MODE_STRINGS) / sizeof(S_MODE_STRINGS[0]))) {
    return S_MODE_STRINGS[mode];
  }

  return PSTR("UNKNOWN");
}

/**
 * @brief Convert error code enum to human-readable ASCII string (stored in Flash).
 *
 * @param code Error code enum.
 * @return const char* Pointer to PROGMEM string representation (e.g. "NONE", "INIT_UART", etc.).
 */
const char *error_handler_code_to_str(error_code_t code) {
  if ((uint8_t)code < (sizeof(S_ERROR_CODE_STRINGS) / sizeof(S_ERROR_CODE_STRINGS[0]))) {
    return S_ERROR_CODE_STRINGS[code];
  }

  return PSTR("UNKNOWN");
}
