/**
 * @file ring_buffer.c
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
 * @brief Lock-free Single-Producer Single-Consumer (SPSC) circular FIFO ring buffer.
 *
 * Designed specifically for 8-bit AVR microcontrollers:
 *  - 8-bit head and tail indexes guarantee atomic read/write without disabling interrupts.
 *  - Enforces power-of-two capacity for 1-cycle bitwise masking (& mask) instead of
 *    expensive multi-cycle software division.
 *  - Capacity is (size - 1) bytes to unambiguously distinguish empty from full states.
 *
 * Circular FIFO Queue Topology:
 *
 *     [0]      [1]      [2]      [3]      ...     [size-2]  [size-1]
 *  +--------+--------+--------+--------+--------+--------+--------+
 *  |  Data  |  Data  |  Data  |  Free  |  Free  |  Free  |  Data  |
 *  +--------+--------+--------+--------+--------+--------+--------+
 *              ^                 ^
 *              |                 |
 *             tail              head
 *       (Read / Pop)      (Write / Push)
 *
 *  - Empty: head == tail
 *  - Full:  ((head + 1) & (size - 1)) == tail
 *  - Count: (head - tail) & (size - 1)
 */

#include "ring_buffer.h"

/**
 * @brief Initialize ring buffer with caller-provided backing memory.
 *
 * Enforces power-of-two capacity for high-performance bitwise wrapping on AVR.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @param storage Pointer to pre-allocated uint8_t array.
 * @param size Total capacity in bytes (MUST be a power of 2: 16, 32, 64, 128).
 * @return bool true on success, false if parameters are invalid or size is not power-of-2.
 */
bool ring_buffer_init(ring_buffer_t *rb, uint8_t *storage, uint8_t size) {
  if (rb == (void *)0 || storage == (void *)0 || size < 2 || (size & (size - 1)) != 0) {
    return false;
  }

  rb->storage = storage;
  rb->size = size;
  rb->head = 0;
  rb->tail = 0;

  return true;
}

/**
 * @brief Push a single byte into the ring buffer.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @param byte Data byte to enqueue.
 * @return bool true if enqueued successfully, false if buffer is full.
 */
bool ring_buffer_push(ring_buffer_t *rb, uint8_t byte) {
  if (rb == (void *)0 || rb->storage == (void *)0 || rb->size == 0) {
    return false;
  }

  uint8_t mask = (uint8_t)(rb->size - 1);
  uint8_t next_head = (uint8_t)((rb->head + 1) & mask);

  if (next_head == rb->tail) {
    return false;
  }

  rb->storage[rb->head] = byte;
  rb->head = next_head;

  return true;
}

/**
 * @brief Pop a single byte from the ring buffer.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @param byte Output pointer where popped byte is stored (may be NULL to discard).
 * @return bool true if dequeued successfully, false if buffer is empty.
 */
bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *byte) {
  if (rb == (void *)0 || rb->storage == (void *)0 || rb->head == rb->tail) {
    return false;
  }

  if (byte != (void *)0) {
    *byte = rb->storage[rb->tail];
  }

  uint8_t mask = (uint8_t)(rb->size - 1);
  rb->tail = (uint8_t)((rb->tail + 1) & mask);

  return true;
}

/**
 * @brief Peek at the next byte without removing it from the buffer.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @param byte Output pointer where peeked byte is stored.
 * @return bool true if byte read, false if buffer is empty.
 */
bool ring_buffer_peek(const ring_buffer_t *rb, uint8_t *byte) {
  if (rb == (void *)0 || rb->storage == (void *)0 || rb->head == rb->tail) {
    return false;
  }

  if (byte != (void *)0) {
    *byte = rb->storage[rb->tail];
  }

  return true;
}

/**
 * @brief Check if the ring buffer is empty.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @return bool true if empty (or rb is NULL), false otherwise.
 */
bool ring_buffer_is_empty(const ring_buffer_t *rb) {
  if (rb == (void *)0) {
    return true;
  }
  return (rb->head == rb->tail);
}

/**
 * @brief Check if the ring buffer is full.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @return bool true if full (or rb is NULL), false otherwise.
 */
bool ring_buffer_is_full(const ring_buffer_t *rb) {
  if (rb == (void *)0 || rb->size == 0) {
    return true;
  }

  uint8_t mask = (uint8_t)(rb->size - 1);

  return (((uint8_t)((rb->head + 1) & mask)) == rb->tail);
}

/**
 * @brief Get the number of elements currently stored in the buffer.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @return uint8_t Number of bytes available.
 */
uint8_t ring_buffer_count(const ring_buffer_t *rb) {
  if (rb == (void *)0 || rb->size == 0) {
    return 0;
  }

  uint8_t mask = (uint8_t)(rb->size - 1);

  return (uint8_t)((rb->head - rb->tail) & mask);
}

/**
 * @brief Reset / clear the ring buffer to empty state.
 *
 * @param rb Pointer to ring_buffer_t instance.
 * @return bool true on success, false if rb is NULL.
 */
bool ring_buffer_clear(ring_buffer_t *rb) {
  if (rb == (void *)0) {
    return false;
  }

  rb->head = 0;
  rb->tail = 0;

  return true;
}
