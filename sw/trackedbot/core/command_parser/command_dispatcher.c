/**
 * @file command_dispatcher.c
 * Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
 *
 * tracked_bot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tracked_bot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * @brief Command router and executor for decoded binary protocol frames.
 *
 * Routes decoded binary protocol packets to dedicated command execution modules
 * under command/, notifies watchdog activity listeners, and handles unknown
 * opcodes.
 *
 * Command Dispatch Table:
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | Command Name      | MSG_ID | Payload  | Subsystem Target      | Response Dispatched      |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | CMD_ID_MOTION     | 0x01   | 1 Byte   | cmd_motion_execute    | RESP_ACK / RESP_NACK     |
 *  | CMD_ID_SET_SPEED  | 0x02   | 1 Byte   | cmd_speed_execute     | RESP_ACK / RESP_NACK     |
 *  | CMD_ID_STOP       | 0x03   | 0 Bytes  | cmd_stop_execute      | RESP_ACK / RESP_NACK     |
 *  | CMD_ID_PING       | 0x04   | 0 Bytes  | cmd_ping_execute      | RESP_PONG                |
 *  | CMD_ID_GET_STATUS | 0x10   | 0 Bytes  | cmd_status_execute    | RESP_STATUS (7 Bytes)    |
 *  | CMD_ID_GET_DIAG   | 0x11   | 0 Bytes  | cmd_diag_execute      | RESP_DIAG (9 Bytes)      |
 *  | CMD_ID_ERR_CLEAR  | 0x12   | 0 Bytes  | cmd_err_clear_execute | RESP_ACK / RESP_NACK     |
 *  | Unknown / Invalid | *      | *        | Error reporting       | RESP_NACK (ERR_UNKNOWN)  |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 */

#include "command_dispatcher.h"
#include "cmd_diag.h"
#include "cmd_err_clear.h"
#include "cmd_motion.h"
#include "cmd_ping.h"
#include "cmd_speed.h"
#include "cmd_status.h"
#include "cmd_stop.h"
#include "telemetry_reporter.h"

/**
 * @brief Invoke activity callback if registered on dispatcher.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @return bool true if activity was notified, false otherwise.
 */
static bool notify_activity(command_dispatcher_t *dispatcher) {
  if (dispatcher != (void *)0 && dispatcher->on_activity != (command_activity_cb_t)0) {
    return dispatcher->on_activity(dispatcher->activity_context);
  }

  return false;
}

/**
 * @brief Initialize command dispatcher with motion controller and stream.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param tracked_motion Pointer to tracked_motion_t kinematics controller.
 * @param stream Communication stream handle (icomm.h).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t
command_dispatcher_init(command_dispatcher_t *dispatcher, tracked_motion_t *tracked_motion, comm_stream_t stream) {
  if (dispatcher == (void *)0 || tracked_motion == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  dispatcher->tracked_motion = tracked_motion;
  dispatcher->stream = stream;
  dispatcher->error_handler = (error_handler_t *)0;
  dispatcher->failsafe = (const failsafe_watchdog_t *)0;
  dispatcher->on_activity = (command_activity_cb_t)0;
  dispatcher->activity_context = (void *)0;

  return MOTION_STATUS_OK;
}

/**
 * @brief Attach error handler to command dispatcher for diagnostic queries.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param eh Pointer to error_handler_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_dispatcher_set_error_handler(command_dispatcher_t *dispatcher, error_handler_t *eh) {
  if (dispatcher == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  dispatcher->error_handler = eh;

  return MOTION_STATUS_OK;
}

/**
 * @brief Attach safety failsafe watchdog to command dispatcher.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param failsafe Pointer to failsafe_watchdog_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_dispatcher_set_failsafe(command_dispatcher_t *dispatcher, const failsafe_watchdog_t *failsafe) {
  if (dispatcher == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  dispatcher->failsafe = failsafe;

  return MOTION_STATUS_OK;
}

/**
 * @brief Register activity callback triggered on valid command reception.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param cb Callback function pointer.
 * @param context User context pointer.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t
command_dispatcher_set_activity_cb(command_dispatcher_t *dispatcher, command_activity_cb_t cb, void *context) {
  if (dispatcher == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  dispatcher->on_activity = cb;
  dispatcher->activity_context = context;

  return MOTION_STATUS_OK;
}

/**
 * @brief Execute a received binary command frame by routing to modular command handlers.
 *
 * @param dispatcher Pointer to command_dispatcher_t instance.
 * @param msg_id Message ID of command (protocol_cmd_id_t).
 * @param payload Pointer to payload buffer (may be NULL if len == 0).
 * @param len Length of payload in bytes.
 * @return bool true if command executed and response sent successfully, false otherwise.
 */
bool command_dispatcher_execute(command_dispatcher_t *dispatcher, uint8_t msg_id, const uint8_t *payload, uint8_t len) {
  if (dispatcher == (void *)0 || dispatcher->tracked_motion == (void *)0) {
    return false;
  }

  (void)notify_activity(dispatcher);

  switch ((protocol_cmd_id_t)msg_id) {
  case CMD_ID_MOTION:
    return cmd_motion_execute(
        dispatcher->tracked_motion, &dispatcher->stream, dispatcher->error_handler, msg_id, payload, len
    );

  case CMD_ID_SET_SPEED:
    return cmd_speed_execute(
        dispatcher->tracked_motion, &dispatcher->stream, dispatcher->error_handler, msg_id, payload, len
    );

  case CMD_ID_STOP:
    return cmd_stop_execute(dispatcher->tracked_motion, &dispatcher->stream, dispatcher->error_handler, msg_id, len);

  case CMD_ID_PING:
    return cmd_ping_execute(&dispatcher->stream);

  case CMD_ID_GET_STATUS:
    return cmd_status_execute(&dispatcher->stream, dispatcher->tracked_motion, dispatcher->error_handler);

  case CMD_ID_GET_DIAG:
    return cmd_diag_execute(
        &dispatcher->stream, dispatcher->tracked_motion, dispatcher->error_handler, dispatcher->failsafe
    );

  case CMD_ID_ERR_CLEAR:
    return cmd_err_clear_execute(dispatcher->error_handler, &dispatcher->stream, msg_id);

  default:
    if (dispatcher->error_handler != (void *)0) {
      (void)error_handler_report(dispatcher->error_handler, ERR_CODE_CMD_UNKNOWN);
    }
    return telemetry_reporter_send_nack(&dispatcher->stream, msg_id, ERR_CODE_CMD_UNKNOWN);
  }
}
