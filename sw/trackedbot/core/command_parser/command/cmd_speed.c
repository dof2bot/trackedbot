/**
 * @file cmd_speed.c
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
 * @brief Implementation of CMD_ID_SET_SPEED (0x02) handler.
 */

#include "cmd_speed.h"
#include "binary_protocol_types.h"
#include "telemetry_reporter.h"

bool cmd_speed_execute(
    tracked_motion_t *motion,
    comm_stream_t *stream,
    error_handler_t *eh,
    uint8_t msg_id,
    const uint8_t *payload,
    uint8_t len
) {
  if (motion == (void *)0 || stream == (void *)0) {
    return false;
  }

  if (len != (uint8_t)PAYLOAD_LEN_SET_SPEED || payload == (void *)0) {
    if (eh != (void *)0) {
      (void)error_handler_report(eh, ERR_CODE_CMD_SYNTAX_ERROR);
    }
    return telemetry_reporter_send_nack(stream, msg_id, ERR_CODE_CMD_SYNTAX_ERROR);
  }

  uint8_t spd = payload[0];
  if (spd < (uint8_t)TRACKED_MOTION_SPEED_MIN) {
    spd = (uint8_t)TRACKED_MOTION_SPEED_MIN;
  }

  (void)tracked_motion_set_speed(motion, spd);
  return telemetry_reporter_send_ack(stream, msg_id);
}
