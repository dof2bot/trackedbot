/**
 * @file telemetry_reporter.h
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
 * @brief Binary telemetry serialization and response dispatch service.
 *
 * Encapsulates outbound protocol frame generation, payload formatting,
 * and CRC-8 checksum calculation for all acknowledgment, diagnostic, and
 * status telemetry packets.
 *
 * Frame Anatomy:
 *  +--------+--------+--------+------------------------+--------+
 *  |  SYNC  | MSG_ID | LENGTH |        PAYLOAD         | CRC-8  |
 *  | (0xAA) | (1 B)  | (1 B)  |       (0..16 B)        | (1 B)  |
 *  +--------+--------+--------+------------------------+--------+
 *           |<--------------- CRC-8 Scope ------------>|
 *
 * Response Registry Summary:
 *  +------------+-------------+---------+----------------------------------------------+
 *  | Response   | MSG_ID      | Payload | Description                                  |
 *  +------------+-------------+---------+----------------------------------------------+
 *  | RESP_ACK   | 0x80        | 1 Byte  | Positive acknowledgment of command ID        |
 *  | RESP_NACK  | 0x81        | 2 Bytes | Rejection with command ID and error_code_t   |
 *  | RESP_PONG  | 0x84        | 0 Bytes | Heartbeat keepalive response                 |
 *  | RESP_STATUS| 0x90        | 7 Bytes | Mode, error, count, motion state, speed      |
 *  | RESP_DIAG  | 0x91        | 9 Bytes | Status (7B) + failsafe flags + motion lock   |
 *  +------------+-------------+---------+----------------------------------------------+
 */

#pragma once

#include "binary_protocol_types.h"
#include "error_handler.h"
#include "failsafe_watchdog.h"
#include "icomm.h"
#include "tracked_motion.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Transmit positive acknowledgment (RESP_ACK: 0x80) for a command.
 *
 * @param stream Communication stream handle.
 * @param cmd_id Message ID of command being acknowledged.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_ack(comm_stream_t *stream, uint8_t cmd_id);

/**
 * @brief Transmit negative acknowledgment (RESP_NACK: 0x81) for a command.
 *
 * @param stream Communication stream handle.
 * @param cmd_id Message ID of command rejected.
 * @param err_code Error code detailing reason for rejection.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_nack(comm_stream_t *stream, uint8_t cmd_id, error_code_t err_code);

/**
 * @brief Transmit heartbeat response (RESP_PONG: 0x84).
 *
 * @param stream Communication stream handle.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_pong(comm_stream_t *stream);

/**
 * @brief Transmit consolidated system status (RESP_STATUS: 0x90).
 *
 * 7-byte binary payload: mode, last_err, count_hi, count_lo, mot_state, mot_cmd, speed.
 *
 * @param stream Communication stream handle.
 * @param motion Tracked motion instance (optional, may be NULL).
 * @param eh Error handler instance (optional, may be NULL).
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_status(comm_stream_t *stream, const tracked_motion_t *motion, const error_handler_t *eh);

/**
 * @brief Transmit full diagnostic information (RESP_DIAG: 0x91).
 *
 * 9-byte binary payload: status (7B) + failsafe_flags (1B) + motion_lock (1B).
 *
 * @param stream Communication stream handle.
 * @param motion Tracked motion instance (optional, may be NULL).
 * @param eh Error handler instance (optional, may be NULL).
 * @param failsafe Failsafe watchdog instance (optional, may be NULL).
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_diag(
    comm_stream_t *stream,
    const tracked_motion_t *motion,
    const error_handler_t *eh,
    const failsafe_watchdog_t *failsafe
);

#ifdef __cplusplus
}
#endif
