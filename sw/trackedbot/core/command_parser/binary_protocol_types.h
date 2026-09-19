/**
 * @file binary_protocol_types.h
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
 * @brief Binary serial protocol type definitions, message IDs, and frame constants.
 *
 * Defines wire-level framing constants, command and response message identifiers,
 * payload length bounds, and diagnostic bitfield flags shared between the binary
 * command parser (command_parser.h), dispatcher (command_dispatcher.h), and
 * telemetry serialization reporter (telemetry_reporter.h).
 *
 * Binary Frame Anatomy:
 *  +--------+--------+--------+--------------------------+--------+
 *  |  SYNC  | MSG_ID | LENGTH |         PAYLOAD          | CRC-8  |
 *  | (0xAA) | (1 B)  | (1 B)  |        (0..16 B)         | (1 B)  |
 *  +--------+--------+--------+--------------------------+--------+
 *           |<--------------- CRC-8 Scope -------------->|
 *
 * Inbound Command Registry (Client -> Robot, Range 0x01..0x7F):
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | Command Enum      | MSG_ID | Payload  | Description           | Expected Response        |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *  | CMD_ID_MOTION     | 0x01   | 1 Byte   | Drive tracks motion   | RESP_ID_ACK / NACK       |
 *  | CMD_ID_SET_SPEED  | 0x02   | 1 Byte   | Set drive speed       | RESP_ID_ACK / NACK       |
 *  | CMD_ID_STOP       | 0x03   | 0 Bytes  | Kinematic stop        | RESP_ID_ACK / NACK       |
 *  | CMD_ID_PING       | 0x04   | 0 Bytes  | Heartbeat query       | RESP_ID_PONG             |
 *  | CMD_ID_GET_STATUS | 0x10   | 0 Bytes  | Query status frame    | RESP_ID_STATUS (7 Bytes) |
 *  | CMD_ID_GET_DIAG   | 0x11   | 0 Bytes  | Query diagnostic frame| RESP_ID_DIAG (9 Bytes)   |
 *  | CMD_ID_ERR_CLEAR  | 0x12   | 0 Bytes  | Clear faults & errors | RESP_ID_ACK / NACK       |
 *  +-------------------+--------+----------+-----------------------+--------------------------+
 *
 * Outbound Response Registry (Robot -> Client, Range 0x80..0xFF):
 *  +-------------------+--------+----------+--------------------------------------------------+
 *  | Response Enum     | MSG_ID | Payload  | Description / Structure                          |
 *  +-------------------+--------+----------+--------------------------------------------------+
 *  | RESP_ID_ACK       | 0x80   | 1 Byte   | Command execution acknowledged: [cmd_id]         |
 *  | RESP_ID_NACK      | 0x81   | 2 Bytes  | Command rejected: [cmd_id, error_code_t]         |
 *  | RESP_ID_PONG      | 0x84   | 0 Bytes  | Heartbeat keepalive response                     |
 *  | RESP_ID_STATUS    | 0x90   | 7 Bytes  | [mode, last_err, count_hi, count_lo,             |
 *  |                   |        |          |  mot_state, mot_cmd, speed]                      |
 *  | RESP_ID_DIAG      | 0x91   | 9 Bytes  | RESP_ID_STATUS (7B) + failsafe_flags + lock_state|
 *  +-------------------+--------+----------+--------------------------------------------------+
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wire-level frame framing delimiters and buffer sizing limits.
 */
enum {
  PROTOCOL_SYNC_BYTE = 0xAA,    /**< Frame start synchronization delimiter (170 dec). */
  PROTOCOL_MAX_PAYLOAD_LEN = 16 /**< Maximum allowable payload length in bytes. */
};

/**
 * @brief Inbound binary protocol command message identifiers (Client -> Robot).
 *
 * Command identifiers occupy the lower 7-bit ID space (0x01..0x7F).
 */
typedef enum {
  CMD_ID_MOTION = 0x01,     /**< Set tracked motion trajectory (Payload: 1B motion_cmd_t). */
  CMD_ID_SET_SPEED = 0x02,  /**< Set motion target PWM speed (Payload: 1B speed 0..255). */
  CMD_ID_STOP = 0x03,       /**< Immediate kinematic stop / deceleration (Payload: 0B). */
  CMD_ID_PING = 0x04,       /**< Keepalive heartbeat ping request (Payload: 0B). */
  CMD_ID_GET_STATUS = 0x10, /**< Telemetry query for consolidated status (Payload: 0B). */
  CMD_ID_GET_DIAG = 0x11,   /**< Diagnostic query for full system health (Payload: 0B). */
  CMD_ID_ERR_CLEAR = 0x12   /**< Clear active error latch and reset counters (Payload: 0B). */
} protocol_cmd_id_t;

/**
 * @brief Outbound binary protocol response message identifiers (Robot -> Client).
 *
 * Response identifiers occupy the upper MSB-set ID space (0x80..0xFF).
 */
typedef enum {
  RESP_ID_ACK = 0x80,    /**< Positive execution acknowledgment (Payload: 1B cmd_id). */
  RESP_ID_NACK = 0x81,   /**< Negative acknowledgment / fault rejection (Payload: 2B). */
  RESP_ID_PONG = 0x84,   /**< Heartbeat pong response (Payload: 0B). */
  RESP_ID_STATUS = 0x90, /**< Standard operating telemetry status packet (Payload: 7B). */
  RESP_ID_DIAG = 0x91    /**< Extended diagnostic status packet (Payload: 9B). */
} protocol_resp_id_t;

/**
 * @brief Fixed payload byte lengths for command and response packets.
 */
enum {
  PAYLOAD_LEN_MOTION = 1,     /**< Payload length for CMD_ID_MOTION (1 byte). */
  PAYLOAD_LEN_SET_SPEED = 1,  /**< Payload length for CMD_ID_SET_SPEED (1 byte). */
  PAYLOAD_LEN_STOP = 0,       /**< Payload length for CMD_ID_STOP (0 bytes). */
  PAYLOAD_LEN_PING = 0,       /**< Payload length for CMD_ID_PING (0 bytes). */
  PAYLOAD_LEN_GET_STATUS = 0, /**< Payload length for CMD_ID_GET_STATUS (0 bytes). */
  PAYLOAD_LEN_GET_DIAG = 0,   /**< Payload length for CMD_ID_GET_DIAG (0 bytes). */
  PAYLOAD_LEN_ERR_CLEAR = 0,  /**< Payload length for CMD_ID_ERR_CLEAR (0 bytes). */
  PAYLOAD_LEN_ACK = 1,        /**< Payload length for RESP_ID_ACK (1 byte). */
  PAYLOAD_LEN_NACK = 2,       /**< Payload length for RESP_ID_NACK (2 bytes). */
  PAYLOAD_LEN_PONG = 0,       /**< Payload length for RESP_ID_PONG (0 bytes). */
  PAYLOAD_LEN_STATUS = 7,     /**< Payload length for RESP_ID_STATUS (7 bytes). */
  PAYLOAD_LEN_DIAG = 9        /**< Payload length for RESP_ID_DIAG (9 bytes). */
};

/**
 * @brief Bitmask flags for communication failsafe watchdog status reporting.
 */
enum {
  FAILSAFE_FLAG_ENABLED = 0x01,  /**< Watchdog timer is actively enabled and arming safety. */
  FAILSAFE_FLAG_TRIGGERED = 0x02 /**< Watchdog timeout has expired, triggering safe stop. */
};

/**
 * @brief Kinematic motion lock operational states.
 */
enum {
  MOTION_LOCK_OFF = 0x00, /**< Kinematic actuator commands are permitted and unlocked. */
  MOTION_LOCK_ON = 0x01   /**< Kinematic actuator commands are locked due to safety/fault. */
};

#ifdef __cplusplus
}
#endif
