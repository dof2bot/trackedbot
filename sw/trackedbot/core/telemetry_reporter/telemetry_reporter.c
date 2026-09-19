/**
 * @file telemetry_reporter.c
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

#include "telemetry_reporter.h"
#include "crc8.h"

/**
 * @brief Construct a binary protocol frame, compute CRC-8, and transmit.
 *
 * Assembles the full packet in a temporary stack buffer of (len + 4) bytes,
 * computes CRC-8 over MSG_ID, LENGTH, and PAYLOAD, and transmits via comm_write_buf.
 *
 * @param stream Communication stream handle.
 * @param msg_id Binary response message identifier.
 * @param payload Pointer to payload byte buffer (may be NULL if len == 0).
 * @param len Number of payload bytes (0 to PROTOCOL_MAX_PAYLOAD_LEN).
 * @return bool true if frame was written successfully to the stream, false otherwise.
 */
static bool send_frame(comm_stream_t *stream, uint8_t msg_id, const uint8_t *payload, uint8_t len) {
  if (stream == (void *)0 || (len > 0 && payload == (void *)0) || len > (uint8_t)PROTOCOL_MAX_PAYLOAD_LEN) {
    return false;
  }

  uint8_t frame[1 + 1 + 1 + PROTOCOL_MAX_PAYLOAD_LEN + 1];
  frame[0] = (uint8_t)PROTOCOL_SYNC_BYTE;
  frame[1] = msg_id;
  frame[2] = len;

  for (uint8_t i = 0; i < len; ++i) {
    frame[3 + i] = payload[i];
  }

  uint8_t crc = crc8_calculate(&frame[1], (uint8_t)(len + 2));
  frame[3 + len] = crc;

  return comm_write_buf(stream, frame, (uint8_t)(len + 4));
}

/**
 * @brief Transmit positive acknowledgment (RESP_ACK: 0x80) for a command.
 *
 * @param stream Communication stream handle.
 * @param cmd_id Message ID of command being acknowledged.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_ack(comm_stream_t *stream, uint8_t cmd_id) {
  return send_frame(stream, (uint8_t)RESP_ID_ACK, &cmd_id, (uint8_t)PAYLOAD_LEN_ACK);
}

/**
 * @brief Transmit negative acknowledgment (RESP_NACK: 0x81) for a command.
 *
 * @param stream Communication stream handle.
 * @param cmd_id Message ID of command rejected.
 * @param err_code Error code detailing reason for rejection.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_nack(comm_stream_t *stream, uint8_t cmd_id, error_code_t err_code) {
  uint8_t payload[2];
  payload[0] = cmd_id;
  payload[1] = (uint8_t)err_code;

  return send_frame(stream, (uint8_t)RESP_ID_NACK, payload, (uint8_t)PAYLOAD_LEN_NACK);
}

/**
 * @brief Transmit heartbeat response (RESP_PONG: 0x84).
 *
 * @param stream Communication stream handle.
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_pong(comm_stream_t *stream) {
  return send_frame(stream, (uint8_t)RESP_ID_PONG, (const uint8_t *)0, (uint8_t)PAYLOAD_LEN_PONG);
}

/**
 * @brief Transmit consolidated system status (RESP_STATUS: 0x90).
 *
 * 7-byte binary payload:
 *  - Byte 0: System mode (NORMAL, DEGRADED, PANIC)
 *  - Byte 1: Last error code
 *  - Bytes 2-3: Total error count (Big-Endian uint16)
 *  - Byte 4: Motion state (IDLE, MOVING, FAILSAFE)
 *  - Byte 5: Active motion command
 *  - Byte 6: Current speed (60 - 255)
 *
 * @param stream Communication stream handle.
 * @param motion Tracked motion instance (optional, may be NULL).
 * @param eh Error handler instance (optional, may be NULL).
 * @return bool true if all stream writes succeeded, false on failure.
 */
bool telemetry_reporter_send_status(comm_stream_t *stream, const tracked_motion_t *motion, const error_handler_t *eh) {
  uint8_t payload[7];

  if (eh != (void *)0) {
    payload[0] = (uint8_t)error_handler_get_mode(eh);
    payload[1] = (uint8_t)error_handler_get_last_error(eh);
    uint16_t count = error_handler_get_error_count(eh);
    payload[2] = (uint8_t)((count >> 8) & 0xFF);
    payload[3] = (uint8_t)(count & 0xFF);
  } else {
    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 0;
    payload[3] = 0;
  }

  if (motion != (void *)0) {
    payload[4] = (uint8_t)tracked_motion_get_state(motion);
    payload[5] = (uint8_t)tracked_motion_get_command(motion);
    payload[6] = tracked_motion_get_speed(motion);
  } else {
    payload[4] = 0;
    payload[5] = 0;
    payload[6] = 0;
  }

  return send_frame(stream, (uint8_t)RESP_ID_STATUS, payload, (uint8_t)PAYLOAD_LEN_STATUS);
}

/**
 * @brief Transmit full diagnostic information (RESP_DIAG: 0x91).
 *
 * 9-byte binary payload:
 *  - Bytes 0-6: Same as RESP_STATUS
 *  - Byte 7: Failsafe flags (bit 0: enabled, bit 1: triggered)
 *  - Byte 8: Motion lock status (0 = allowed, 1 = locked)
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
) {
  uint8_t payload[9];

  if (eh != (void *)0) {
    payload[0] = (uint8_t)error_handler_get_mode(eh);
    payload[1] = (uint8_t)error_handler_get_last_error(eh);
    uint16_t count = error_handler_get_error_count(eh);
    payload[2] = (uint8_t)((count >> 8) & 0xFF);
    payload[3] = (uint8_t)(count & 0xFF);
  } else {
    payload[0] = 0;
    payload[1] = 0;
    payload[2] = 0;
    payload[3] = 0;
  }

  if (motion != (void *)0) {
    payload[4] = (uint8_t)tracked_motion_get_state(motion);
    payload[5] = (uint8_t)tracked_motion_get_command(motion);
    payload[6] = tracked_motion_get_speed(motion);
  } else {
    payload[4] = 0;
    payload[5] = 0;
    payload[6] = 0;
  }

  uint8_t flags = 0;
  if (failsafe != (void *)0) {
    if (failsafe_is_enabled(failsafe)) {
      flags |= (uint8_t)FAILSAFE_FLAG_ENABLED;
    }
    if (failsafe_is_triggered(failsafe)) {
      flags |= (uint8_t)FAILSAFE_FLAG_TRIGGERED;
    }
  }
  payload[7] = flags;

  if (eh != (void *)0 && !error_handler_is_motion_allowed(eh)) {
    payload[8] = (uint8_t)MOTION_LOCK_ON;
  } else {
    payload[8] = (uint8_t)MOTION_LOCK_OFF;
  }

  return send_frame(stream, (uint8_t)RESP_ID_DIAG, payload, (uint8_t)PAYLOAD_LEN_DIAG);
}
