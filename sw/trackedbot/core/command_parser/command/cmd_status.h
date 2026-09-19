/**
 * @file cmd_status.h
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
 * @brief Discrete handler for CMD_ID_GET_STATUS (0x10) binary frame.
 */

#pragma once

#include "error_handler.h"
#include "icomm.h"
#include "tracked_motion.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute query status command frame.
 *
 * @param stream Pointer to outbound communication stream handle.
 * @param motion Pointer to tracked motion kinematics instance.
 * @param eh Optional pointer to error handler instance.
 * @return bool true if status telemetry packet was transmitted, false otherwise.
 */
bool cmd_status_execute(
    comm_stream_t *stream,
    const tracked_motion_t *motion,
    const error_handler_t *eh
);

#ifdef __cplusplus
}
#endif
