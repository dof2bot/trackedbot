/**
 * @file cmd_err_clear.h
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
 * @brief Discrete handler for CMD_ID_ERR_CLEAR (0x12) binary frame.
 */

#pragma once

#include "error_handler.h"
#include "icomm.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execute clear errors command frame.
 *
 * @param eh Pointer to error handler instance.
 * @param stream Pointer to outbound communication stream handle.
 * @param msg_id Command message ID (CMD_ID_ERR_CLEAR).
 * @return bool true if error state cleared and response sent, false otherwise.
 */
bool cmd_err_clear_execute(
    error_handler_t *eh,
    comm_stream_t *stream,
    uint8_t msg_id
);

#ifdef __cplusplus
}
#endif
