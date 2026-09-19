/**
 * @file command_parser.h
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

#pragma once

#include "binary_protocol_types.h"
#include "command_dispatcher.h"
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
 * @brief Discrete operational states of the binary packet parser state machine.
 */
typedef enum {
  PARSER_STATE_WAIT_SYNC = 0, /**< Waiting for SYNC preamble byte (0xAA). */
  PARSER_STATE_READ_ID,       /**< Awaiting 1-byte command MSG_ID. */
  PARSER_STATE_READ_LEN,      /**< Awaiting 1-byte payload length field (0..16). */
  PARSER_STATE_READ_PAYLOAD,  /**< Buffering incoming payload data bytes. */
  PARSER_STATE_CHECK_CRC      /**< Awaiting and validating CRC-8 checksum byte. */
} parser_state_t;

/**
 * @brief Context structure for binary command frame parser.
 */
typedef struct {
  command_dispatcher_t dispatcher;                  /**< Embedded command dispatcher instance. */
  parser_state_t state;                             /**< Current state of the parser FSM. */
  uint8_t msg_id;                                   /**< Message ID of active frame being assembled. */
  uint8_t payload_len;                              /**< Expected payload length of active frame. */
  uint8_t payload_index;                            /**< Current byte write index into payload buffer. */
  uint8_t payload_buffer[PROTOCOL_MAX_PAYLOAD_LEN]; /**< Buffer for storing incoming frame payload. */
} command_parser_t;

/**
 * @brief Initialize binary command parser with kinematics and stream handle.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param tracked_motion Pointer to tracked_motion_t kinematics controller.
 * @param stream Abstract communication stream handle (icomm.h).
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_init(command_parser_t *parser, tracked_motion_t *tracked_motion, comm_stream_t stream);

/**
 * @brief Attach error handler to parser for diagnostic queries and error reporting.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param eh Pointer to error_handler_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if parser is NULL.
 */
motion_status_t command_parser_set_error_handler(command_parser_t *parser, error_handler_t *eh);

/**
 * @brief Attach safety failsafe watchdog to parser for status queries.
 *
 * @param parser Pointer to command_parser_t instance.
 * @param failsafe Pointer to failsafe_watchdog_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG if parser is NULL.
 */
motion_status_t command_parser_set_failsafe(command_parser_t *parser, const failsafe_watchdog_t *failsafe);

/**
 * @brief Register a callback triggered whenever a valid command is received (for failsafe).
 *
 * @param parser Pointer to command_parser_t instance.
 * @param cb Callback function pointer.
 * @param context User context pointer passed to callback.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_set_activity_cb(command_parser_t *parser, command_activity_cb_t cb, void *context);

/**
 * @brief Process incoming bytes from the communication stream via binary state machine.
 *
 * Synchronizes on 0xAA, reads MSG_ID and LEN, collects PAYLOAD, and validates CRC-8.
 * Valid packets are dispatched to command_dispatcher_execute().
 *
 * @param parser Pointer to command_parser_t instance.
 * @return motion_status_t MOTION_STATUS_OK on success, MOTION_STATUS_INVALID_ARG on error.
 */
motion_status_t command_parser_process(command_parser_t *parser);

#ifdef __cplusplus
}
#endif
