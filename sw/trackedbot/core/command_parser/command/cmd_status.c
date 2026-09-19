/**
 * @file cmd_status.c
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
 * @brief Implementation of CMD_ID_GET_STATUS (0x10) handler.
 */

#include "cmd_status.h"
#include "telemetry_reporter.h"

bool cmd_status_execute(
    comm_stream_t *stream,
    const tracked_motion_t *motion,
    const error_handler_t *eh
) {
  if (stream == (void *)0 || motion == (void *)0) {
    return false;
  }

  return telemetry_reporter_send_status(stream, motion, eh);
}
