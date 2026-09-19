/**
 * @file cmd_motion.h
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
 * @brief Discrete handler for CMD_ID_MOTION (0x01) binary frame.
 */

#pragma once

#include "error_handler.h"
#include "icomm.h"
#include "tracked_motion.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute motion command frame.
 *
 * @param motion Pointer to tracked motion kinematics instance.
 * @param stream Pointer to outbound communication stream handle.
 * @param eh Optional pointer to error handler instance.
 * @param msg_id Command message ID (CMD_ID_MOTION).
 * @param payload Pointer to payload buffer containing motion direction.
 * @param len Payload length in bytes (must equal PAYLOAD_LEN_MOTION = 1).
 * @return bool true if command executed and response transmitted, false otherwise.
 */
bool cmd_motion_execute(
    tracked_motion_t *motion,
    comm_stream_t *stream,
    error_handler_t *eh,
    uint8_t msg_id,
    const uint8_t *payload,
    uint8_t len
);

#ifdef __cplusplus
}
#endif
