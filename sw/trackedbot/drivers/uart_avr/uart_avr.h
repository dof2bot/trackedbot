/**
 * @file uart_avr.h
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
 */

#pragma once

#include "icomm.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART configuration parameters.
 */
typedef enum {
  UART_DEFAULT_BAUD_RATE = 9600UL,
  UART_RX_BUFFER_SIZE = 64,
  UART_TX_BUFFER_SIZE = 64
} uart_config_t;

/**
 * @brief Initialize USART0 hardware and ring buffers.
 *
 * @param baud_rate Communication speed in bps (e.g. 115200 or 9600).
 * @return bool true on success, false if baud rate is invalid or buffer init fails.
 */
bool uart_avr_init(uint32_t baud_rate);

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @return bool true if at least one byte is available to read.
 */
bool uart_avr_available(void);

/**
 * @brief Read a single byte from the receive ring buffer (returns 0 if buffer is empty).
 *
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
uint8_t uart_avr_read(void);

/**
 * @brief Non-blocking transmit byte over USART0 via TX ring buffer.
 *
 * Enqueues the byte in the software buffer and activates UDRIE0 interrupt
 * to initiate asynchronous transmission.
 *
 * @param byte Byte to transmit.
 * @return bool true if enqueued successfully.
 */
bool uart_avr_write(uint8_t byte);

/**
 * @brief Transmit a null-terminated string residing in SRAM.
 *
 * @param str Pointer to null-terminated RAM string.
 * @return bool true if all bytes enqueued successfully.
 */
bool uart_avr_write_str(const char *str);

/**
 * @brief Transmit a null-terminated string directly from Flash (PROGMEM).
 *
 * Reads bytes from Program Flash memory using LPM instruction, streaming
 * directly into the TX buffer with zero SRAM consumption.
 *
 * @param progmem_str Pointer to Flash memory string.
 * @return bool true if all bytes enqueued successfully.
 */
bool uart_avr_write_str_P(const char *progmem_str);

/**
 * @brief Block until all queued TX data is flushed and physical shift register is empty.
 *
 * @return bool true when transmission is completely finished.
 */
bool uart_avr_flush_tx(void);

/**
 * @brief Check if transmit buffer is empty and hardware transmitter is idle.
 *
 * @return bool true if no transmission is in flight.
 */
bool uart_avr_tx_complete(void);

/**
 * @brief Factory function producing an abstract communication stream handle.
 *
 * Adapts USART0 driver to the icomm.h interface for Dependency Injection
 * into the command parser and telemetry subsystems.
 *
 * @return comm_stream_t Abstract stream interface structure.
 */
comm_stream_t uart_avr_get_stream(void);

#ifdef __cplusplus
}
#endif
