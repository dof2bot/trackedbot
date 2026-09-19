/**
 * @file command_dispatcher.h
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
 * @brief Command router and executor for decoded binary protocol frames.
 *
 * Validates payload parameters, verifies syntax bounds, notifies activity
 * watchdog listeners, invokes core subsystems (motion kinematics, error handler),
 * and dispatches corresponding telemetry response packets.
 *
 * Command Dispatch Table:
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | Command Name      | MSG_ID | Payload  | Subsystem Target      | Response Dispatched      |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | CMD_ID_MOTION     | 0x01   | 1 Byte   | tracked_motion_set_cmd| RESP_ACK / RESP_NACK     |
 *  | CMD_ID_SET_SPEED  | 0x02   | 1 Byte   | tracked_motion_set_spd| RESP_ACK / RESP_NACK     |
 *  | CMD_ID_STOP       | 0x03   | 0 Bytes  | tracked_motion_stop   | RESP_ACK / RESP_NACK     |
 *  | CMD_ID_PING       | 0x04   | 0 Bytes  | Keepalive heartbeat   | RESP_PONG                |
 *  | CMD_ID_GET_STATUS | 0x10   | 0 Bytes  | Status query          | RESP_STATUS (7 Bytes)    |
 *  | CMD_ID_GET_DIAG   | 0x11   | 0 Bytes  | Diagnostic query      | RESP_DIAG (9 Bytes)      |
 *  | CMD_ID_ERR_CLEAR  | 0x20   | 0 Bytes  | error_handler_clear   | RESP_ACK / RESP_NACK     |
 *  | Unknown / Invalid | *      | *        | Error reporting       | RESP_NACK (ERR_UNKNOWN)  |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 */

#pragma once

#include "binary_protocol_types.h"
#include "error_handler.h"
#include "failsafe_watchdog.h"
#include "icomm.h"
#include "motion_types.h"
#include "tracked_motion.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function pointer callback type invoked upon any valid command activity.
 *
 * Used to reset the communication failsafe watchdog timer.
 *
 * @param context User context pointer.
 * @return bool true if activity was successfully processed, false otherwise.
 */
typedef bool (*command_activity_cb_t)(void *context);

/**
 * @brief Context structure for binary command dispatcher.
 */
typedef struct {
  tracked_motion_t *tracked_motion;    /**< Pointer to tracked motion kinematics controller. */
  comm_stream_t stream;                /**< Outbound communication stream handle. */
  error_handler_t *error_handler;      /**< Optional pointer to error reporting handler. */
  const failsafe_watchdog_t *failsafe; /**< Optional pointer to failsafe watchdog instance. */
  command_activity_cb_t on_activity;   /**< Optional callback invoked on valid command reception. */
  void *activity_context;              /**< User context pointer passed to on_activity callback. */
} command_dispatcher_t;

/**
 * @brief Initialize command dispatcher with motion controller and stream.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param tracked_motion Pointer to tracked_motion_t kinematics controller.
 * @param stream Communication stream handle (icomm.h).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t
command_dispatcher_init(command_dispatcher_t *dispatcher, tracked_motion_t *tracked_motion, comm_stream_t stream);

/**
 * @brief Attach error handler to command dispatcher for diagnostic queries.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param eh Pointer to error_handler_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_dispatcher_set_error_handler(command_dispatcher_t *dispatcher, error_handler_t *eh);

/**
 * @brief Attach safety failsafe watchdog to command dispatcher.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param failsafe Pointer to failsafe_watchdog_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_dispatcher_set_failsafe(command_dispatcher_t *dispatcher, const failsafe_watchdog_t *failsafe);

/**
 * @brief Register activity callback triggered on valid command reception.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param cb Callback function pointer.
 * @param context User context pointer.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t
command_dispatcher_set_activity_cb(command_dispatcher_t *dispatcher, command_activity_cb_t cb, void *context);

/**
 * @brief Execute a received binary command frame using O(1) switch dispatch.
 *
 * Validates payload parameters, notifies activity callback, routes action to
 * target subsystem, and emits acknowledgment or diagnostic telemetry response.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param msg_id Message ID of command (protocol_cmd_id_t).
 * @param payload Pointer to payload buffer (may be NULL if len == 0).
 * @param len Length of payload in bytes.
 * @return bool true if command executed and response sent successfully, false otherwise.
 */
bool command_dispatcher_execute(command_dispatcher_t *dispatcher, uint8_t msg_id, const uint8_t *payload, uint8_t len);

#ifdef __cplusplus
}
#endif
