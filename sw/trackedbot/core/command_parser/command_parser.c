/**
 * @file command_parser.c
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
 * @brief Binary serial protocol frame parser implementing a 5-state FSM.
 *
 * Consumes raw byte streams from the communication layer (icomm.h), detects
 * frame boundaries (SYNC 0xAA), extracts message headers and payload, validates
 * CRC-8 checksums, and forwards verified command frames to the dispatcher.
 *
 * Binary Frame Parser State Machine (FSM):
 *
 *       +------------------------+
 *  +--->| PARSER_STATE_WAIT_SYNC |<--------------------------+ (Invalid / Reset)
 *  |    +-----------+------------+                           |
 *  |                | byte == 0xAA (PROTOCOL_SYNC_BYTE)      |
 *  |                v                                        |
 *  |    +------------------------+                           |
 *  |    |  PARSER_STATE_READ_ID  |                           |
 *  |    +-----------+------------+                           |
 *  |                | store msg_id                           |
 *  |                v                                        |
 *  |    +------------------------+                           |
 *  |    | PARSER_STATE_READ_LEN  |                           |
 *  |    +-----------+------------+                           |
 *  |                |                                        |
 *  |       +--------+--------+                               |
 *  |       | len > MAX       | len == 0                      |
 *  |       v                 v                               |
 *  |     [NACK]    +------------------------+                |
 *  |               | PARSER_STATE_CHECK_CRC |<-------+       |
 *  |               +------------+-----------+        |       |
 *  |                            |                    |       |
 *  |          len > 0           |                    |       |
 *  |                            v                    |       |
 *  |               +----------------------------+    |       |
 *  |               |  PARSER_STATE_READ_PAYLOAD |----+       |
 *  |               +----------------------------+            |
 *  |                     (index == len)                      |
 *  |                                                         |
 *  |  CRC Match: dispatch command -> [ACK/RESP]              |
 *  |  CRC Mismatch: report error  -> [NACK]                  |
 *  +---------------------------------------------------------+
 */

#include "command_parser.h"
#include "crc8.h"
#include "telemetry_reporter.h"

/**
 * @brief Initialize binary command parser with kinematics and stream handle.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param tracked_motion Pointer to tracked_motion_t kinematics controller.
 * @param stream Abstract communication stream handle (icomm.h).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_init(command_parser_t *parser, tracked_motion_t *tracked_motion, comm_stream_t stream) {
  if (parser == (void *)0 || tracked_motion == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  parser->state = PARSER_STATE_WAIT_SYNC;
  parser->msg_id = 0;
  parser->payload_len = 0;
  parser->payload_index = 0;

  return command_dispatcher_init(&parser->dispatcher, tracked_motion, stream);
}

/**
 * @brief Attach error handler to parser for diagnostic queries and error reporting.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param eh Pointer to error_handler_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if parser is NULL.
 */
motion_status_t command_parser_set_error_handler(command_parser_t *parser, error_handler_t *eh) {
  if (parser == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return command_dispatcher_set_error_handler(&parser->dispatcher, eh);
}

/**
 * @brief Attach safety failsafe watchdog to parser for status queries.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param failsafe Pointer to failsafe_watchdog_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if parser is NULL.
 */
motion_status_t command_parser_set_failsafe(command_parser_t *parser, const failsafe_watchdog_t *failsafe) {
  if (parser == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return command_dispatcher_set_failsafe(&parser->dispatcher, failsafe);
}

/**
 * @brief Register a callback triggered whenever a valid command is received (for failsafe).
 *
 * @param parser Pointer to command_parser_t instance.
 * @param cb Callback function pointer.
 * @param context User context pointer passed to callback.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_set_activity_cb(command_parser_t *parser, command_activity_cb_t cb, void *context) {
  if (parser == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  return command_dispatcher_set_activity_cb(&parser->dispatcher, cb, context);
}

/**
 * @brief Process incoming bytes from the communication stream via binary state machine.
 *
 * Synchronizes on 0xAA, reads MSG_ID and LEN, collects PAYLOAD, and validates CRC-8.
 * Valid packets are dispatched to command_dispatcher_execute().
 *
 * @param parser Pointer to command_parser_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_process(command_parser_t *parser) {
  if (parser == (void *)0 || parser->dispatcher.tracked_motion == (void *)0) {
    return MOTION_STATUS_INVALID_ARG;
  }

  while (comm_available(&parser->dispatcher.stream)) {
    uint8_t ch = comm_read(&parser->dispatcher.stream);

    switch (parser->state) {
    case PARSER_STATE_WAIT_SYNC:
      if (ch == (uint8_t)PROTOCOL_SYNC_BYTE) {
        parser->state = PARSER_STATE_READ_ID;
      }
      break;

    case PARSER_STATE_READ_ID:
      parser->msg_id = ch;
      parser->state = PARSER_STATE_READ_LEN;
      break;

    case PARSER_STATE_READ_LEN:
      parser->payload_len = ch;
      parser->payload_index = 0;
      if (parser->payload_len > (uint8_t)PROTOCOL_MAX_PAYLOAD_LEN) {
        if (parser->dispatcher.error_handler != (void *)0) {
          (void)error_handler_report(parser->dispatcher.error_handler, ERR_CODE_CMD_SYNTAX_ERROR);
        }
        (void)telemetry_reporter_send_nack(&parser->dispatcher.stream, parser->msg_id, ERR_CODE_CMD_SYNTAX_ERROR);
        parser->state = PARSER_STATE_WAIT_SYNC;
      } else if (parser->payload_len == 0) {
        parser->state = PARSER_STATE_CHECK_CRC;
      } else {
        parser->state = PARSER_STATE_READ_PAYLOAD;
      }
      break;

    case PARSER_STATE_READ_PAYLOAD:
      parser->payload_buffer[parser->payload_index++] = ch;
      if (parser->payload_index >= parser->payload_len) {
        parser->state = PARSER_STATE_CHECK_CRC;
      }
      break;

    case PARSER_STATE_CHECK_CRC: {
      uint8_t expected_crc = crc8_update(0x00, parser->msg_id);
      expected_crc = crc8_update(expected_crc, parser->payload_len);
      for (uint8_t i = 0; i < parser->payload_len; ++i) {
        expected_crc = crc8_update(expected_crc, parser->payload_buffer[i]);
      }

      if (ch == expected_crc) {
        (void
        )command_dispatcher_execute(&parser->dispatcher, parser->msg_id, parser->payload_buffer, parser->payload_len);
      } else {
        if (parser->dispatcher.error_handler != (void *)0) {
          (void)error_handler_report(parser->dispatcher.error_handler, ERR_CODE_COMM_CRC);
        }
        (void)telemetry_reporter_send_nack(&parser->dispatcher.stream, parser->msg_id, ERR_CODE_COMM_CRC);
      }

      parser->state = PARSER_STATE_WAIT_SYNC;
    } break;

    default:
      parser->state = PARSER_STATE_WAIT_SYNC;
      break;
    }
  }

  return MOTION_STATUS_OK;
}
