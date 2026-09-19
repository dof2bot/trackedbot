/**
 * @file cmd_err_clear.c
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
 * @brief Implementation of CMD_ID_ERR_CLEAR (0x12) handler.
 */

#include "cmd_err_clear.h"
#include "binary_protocol_types.h"
#include "telemetry_reporter.h"

bool cmd_err_clear_execute(
    error_handler_t *eh,
    comm_stream_t *stream,
    uint8_t msg_id
) {
  if (stream == (void *)0) {
    return false;
  }

  if (eh != (void *)0) {
    (void)error_handler_clear(eh);
    return telemetry_reporter_send_ack(stream, msg_id);
  }

  return telemetry_reporter_send_nack(stream, msg_id, ERR_CODE_HARDWARE_FAULT);
}
