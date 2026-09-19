/**
 * @file icomm.h
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
 * @brief Abstract communication stream interface (Output Port / DIP).
 *
 * Defines the bidirectional communication abstraction conforming to the
 * Dependency Inversion Principle (DIP) in Clean Architecture. Decouples
 * high-level command parsing and telemetry serialization from concrete
 * hardware transports (USART0, ESP8266 WiFi, Bluetooth, or unit-test mocks).
 *
 * Ports & Adapters Architectural Flow:
 *
 *    +--------------------+       +--------------------+
 *    |   command_parser   |       | telemetry_reporter |   [ CORE / DOMAIN LAYER ]
 *    +--------------------+       +--------------------+
 *               |                            |
 *               +--------------+-------------+
 *                              |
 *                              v (Depends strictly on interface)
 *              =================================
 *              PORT: comm_stream_t (icomm.h)
 *                - available()
 *                - read()
 *                - write()
 *                - write_str()
 *                - write_str_p()
 *              =================================
 *                              ^
 *                              |
 *               +--------------+-------------+
 *               |                            |
 *    +--------------------+       +--------------------+
 *    |   uart_avr (HW)    |       | mock_stream (Test) |   [ ADAPTERS / INFRASTRUCTURE ]
 *    +--------------------+       +--------------------+
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "progmem_compat.h"

/**
 * @brief Abstract communication stream interface (Output Port / DIP).
 *
 * This interface allows dependency injection of different communication
 * transports (UART, WiFi, BLE, mock stream) without modifying domain logic.
 *
 * @param context Pointer to stream-specific context data.
 * @param available Function to check if data is available to read.
 * @param read Function to read a single byte from the stream.
 * @param write Function to write a single byte to the stream.
 * @param write_str Function to write a null-terminated string to the stream.
 * @param write_str_p Function to write a null-terminated string from Flash to the stream.
 */
typedef struct {
  void *context;
  bool (*available)(void *context);
  uint8_t (*read)(void *context);
  bool (*write)(void *context, uint8_t byte);
  bool (*write_str)(void *context, const char *str);
  bool (*write_str_p)(void *context, const char *flash_str);
} comm_stream_t;

/**
 * @brief Check if there is data available to read.
 *
 * @param stream Communication stream pointer.
 * @return bool true if data available to read, false otherwise.
 */
static inline bool comm_available(const comm_stream_t *stream) {
  if (stream != (void *)0 && stream->available != (void *)0) {
    return stream->available(stream->context);
  }

  return false;
}

/**
 * @brief Read a single byte from the stream.
 *
 * @param stream Communication stream pointer.
 * @return uint8_t Next available byte, or 0 if empty or stream is invalid.
 */
static inline uint8_t comm_read(const comm_stream_t *stream) {
  if (stream != (void *)0 && stream->read != (void *)0) {
    return stream->read(stream->context);
  }

  return 0;
}

/**
 * @brief Write a single byte to the stream.
 *
 * @param stream Communication stream pointer.
 * @param byte Byte to transmit.
 * @return bool true on success, false on error.
 */
static inline bool comm_write(const comm_stream_t *stream, uint8_t byte) {
  if (stream != (void *)0 && stream->write != (void *)0) {
    return stream->write(stream->context, byte);
  }

  return false;
}

/**
 * @brief Write a buffer of bytes to the stream.
 *
 * @param stream Communication stream pointer.
 * @param buf Pointer to byte buffer.
 * @param len Number of bytes to write.
 * @return bool true if all bytes written successfully, false otherwise.
 */
static inline bool comm_write_buf(const comm_stream_t *stream, const uint8_t *buf, uint8_t len) {
  if (stream != (void *)0 && buf != (void *)0) {
    bool ok = true;

    for (uint8_t i = 0; i < len; ++i) {
      if (!comm_write(stream, buf[i])) {
        ok = false;
      }
    }

    return ok;
  }

  return false;
}

/**
 * @brief Write a null-terminated string to the stream.
 *
 * Uses stream->write_str if provided, or falls back to byte-by-byte write().
 *
 * @param stream Communication stream pointer.
 * @param str Pointer to null-terminated RAM string.
 * @return bool true on success, false on error.
 */
static inline bool comm_write_str(const comm_stream_t *stream, const char *str) {
  if (stream != (void *)0) {
    if (stream->write_str != (void *)0) {
      return stream->write_str(stream->context, str);
    } else if (stream->write != (void *)0 && str != (void *)0) {
      bool ok = true;

      while (*str != '\0') {
        if (!stream->write(stream->context, (uint8_t)(*str))) {
          ok = false;
        }

        str++;
      }

      return ok;
    }
  }

  return false;
}

/**
 * @brief Write a null-terminated string stored in Flash (PROGMEM) to the stream.
 *
 * Uses stream->write_str_p if provided, or falls back to LPM byte-by-byte write().
 *
 * @param stream Communication stream pointer.
 * @param flash_str Pointer to string located in Flash memory.
 * @return bool true on success, false on error.
 */
static inline bool comm_write_str_p(const comm_stream_t *stream, const char *flash_str) {
  if (stream != (void *)0 && flash_str != (void *)0) {
    if (stream->write_str_p != (void *)0) {
      return stream->write_str_p(stream->context, flash_str);
    } else if (stream->write != (void *)0) {
      bool ok = true;

      while (1) {
        uint8_t b = (uint8_t)pgm_read_byte(flash_str++);

        if (b == 0) {
          break;
        }

        if (!stream->write(stream->context, b)) {
          ok = false;
        }
      }

      return ok;
    }
  }

  return false;
}

#ifdef __cplusplus
}
#endif
