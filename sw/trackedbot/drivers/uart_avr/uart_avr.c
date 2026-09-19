/**
 * @file uart_avr.c
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
 * @brief Hardware USART0 asynchronous serial transceiver with interrupt ring buffers.
 *
 * Implements non-blocking, interrupt-driven bidirectional serial communication
 * over ATmega328P USART0 (Pin 0/PD0 RXD, Pin 1/PD1 TXD).
 *
 * Baud Rate Generator Formula (Asynchronous Normal Mode, U2X0 = 0):
 *
 *             f_CPU
 *   UBRR0 = -------------  -  1
 *           16 * BAUD_RATE
 *
 *   Examples @ f_CPU = 16.000000 MHz:
 *    - 115,200 bps: UBRR0 = (16,000,000 / (16 * 115,200)) - 1 = 7.68 -> 8  (-3.5% error)
 *    -  57,600 bps: UBRR0 = (16,000,000 / (16 *  57,600)) - 1 = 16.36 -> 16 (+2.1% error)
 *    -   9,600 bps: UBRR0 = (16,000,000 / (16 *   9,600)) - 1 = 103.16 -> 103 (+0.16% error)
 *
 * 8N1 Frame Format (1 Start, 8 Data, No Parity, 1 Stop):
 *
 *  IDLE   START   D0    D1    D2    D3    D4    D5    D6    D7   STOP   IDLE
 *  -----+       +-----+-----+-----+-----+-----+-----+-----+-----+-----+------
 *       |       | LSB |  .  |  .  |  .  |  .  |  .  |  .  | MSB |     |
 *       +-------+-----+-----+-----+-----+-----+-----+-----+-----+-----+
 *       |<-1b ->|<------------------ 8 Data Bits -------------->|<-1b->|
 *       | Start |                                               | Stop |
 *
 * Hardware Architecture & Data Flow:
 *
 *                                  +---------------------------+
 *                                  |  USART_RX_vect Interrupt  |
 *                                  +---------------------------+
 *                                                |
 *                                        (Pushes byte into)
 *                                                v
 *  RX Pin (PD0) ---> [ UDR0 Shift Reg ] ---> [ s_rx_ring (64B) ] ---> uart_avr_read()
 *
 *  TX Pin (PD1) <--- [ UDR0 Shift Reg ] <--- [ s_tx_ring (64B) ] <--- uart_avr_write()
 *                                                ^
 *                                        (Pops byte from)
 *                                                |
 *                                  +---------------------------+
 *                                  | USART_UDRE_vect Interrupt |
 *                                  +---------------------------+
 *
 * Interrupt Lifecycle:
 *  - USART_RX_vect: Triggered by hardware when RXC0 is set (byte received).
 *                   Reads UDR0 and pushes into s_rx_ring without blocking.
 *  - USART_UDRE_vect: Triggered when UDR0 is empty and ready for the next byte.
 *                     Pops from s_tx_ring into UDR0. Automatically clears UDRIE0
 *                     when buffer becomes empty to prevent CPU lockup.
 */

#include "uart_avr.h"
#include "ring_buffer.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* Ring buffer memory for RX FIFO and TX FIFO. */
static uint8_t s_rx_storage[UART_RX_BUFFER_SIZE];
static ring_buffer_t s_rx_ring;
static uint8_t s_tx_storage[UART_TX_BUFFER_SIZE];
static ring_buffer_t s_tx_ring;

/**
 * @brief USART Receive Complete Interrupt Service Routine.
 *
 * Pushes the received data byte from the hardware data register (UDR0) into the
 * RX ring buffer.
 *
 * @note Automatically invoked by AVR hardware when the RXC0 flag is set.
 */
ISR(USART_RX_vect) { (void)ring_buffer_push(&s_rx_ring, UDR0); }

/**
 * @brief USART Data Register Empty Interrupt Service Routine.
 *
 * Feeds the hardware transmit register (UDR0) from the software TX ring buffer.
 *
 * @note Triggered when UDR0 is empty. Disables UDRIE0 when the TX ring buffer
 *       becomes empty to prevent continuous interrupt re-triggering.
 */
ISR(USART_UDRE_vect) {
  uint8_t data = 0;

  if (ring_buffer_pop(&s_tx_ring, &data)) {
    UDR0 = data;
  } else {
    /* CRITICAL SECTION: No more data to transmit; disable Data Register Empty Interrupt. */
    UCSR0B &= ~(1 << UDRIE0);
  }
}

/**
 * @brief Initialize USART0 hardware and ring buffers.
 *
 * @param baud_rate Communication speed in bps (e.g. 115200 or 9600).
 * @return bool true on success, false if baud rate is invalid or buffer init fails.
 */
bool uart_avr_init(uint32_t baud_rate) {
  if (baud_rate == 0) {
    return false;
  }

  /* Initialize dedicated ring buffer components for RX FIRST. */
  if (!ring_buffer_init(&s_rx_ring, s_rx_storage, (uint8_t)sizeof(s_rx_storage))) {
    return false;
  }

  /* Initialize dedicated ring buffer components for TX FIRST. */
  if (!ring_buffer_init(&s_tx_ring, s_tx_storage, (uint8_t)sizeof(s_tx_storage))) {
    return false;
  }

  /* Configure USART hardware. */
  uint16_t ubrr = (uint16_t)(((F_CPU / (16UL * baud_rate))) - 1UL);

  /* Set baud rate registers. */
  UBRR0H = (uint8_t)(ubrr >> 8);
  UBRR0L = (uint8_t)(ubrr);

  /* Reset double speed mode. */
  UCSR0A &= ~(1 << U2X0);

  /* Enable Receiver, Transmitter and RX Complete Interrupt (UDRIE disabled initially). */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

  /* Set frame format: 8 data bits, 1 stop bit, no parity (8N1). */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

  return true;
}

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @return bool true if at least one byte is available to read.
 */
bool uart_avr_available(void) { return !ring_buffer_is_empty(&s_rx_ring); }

/**
 * @brief Read a single byte from the receive ring buffer (returns 0 if buffer is empty).
 *
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
uint8_t uart_avr_read(void) {
  uint8_t data = 0;

  (void)ring_buffer_pop(&s_rx_ring, &data);

  return data;
}

/**
 * @brief Non-blocking transmit byte over USART0 via TX ring buffer.
 *
 * Enqueues the byte in the software buffer and activates UDRIE0 interrupt
 * to initiate asynchronous transmission.
 *
 * @param byte Byte to transmit.
 * @return bool true if enqueued successfully.
 */
bool uart_avr_write(uint8_t byte) {
  /* If transmit buffer is full, ensure interrupt is active and wait for slot */
  while (ring_buffer_is_full(&s_tx_ring)) {
    UCSR0B |= (1 << UDRIE0);
  }

  bool ok = ring_buffer_push(&s_tx_ring, byte);

  /* Trigger interrupt-driven transmission */
  UCSR0B |= (1 << UDRIE0);

  return ok;
}

/**
 * @brief Transmit a null-terminated string residing in SRAM.
 *
 * @param str Pointer to null-terminated RAM string.
 * @return bool true if all bytes enqueued successfully.
 */
bool uart_avr_write_str(const char *str) {
  if (str == (void *)0) {
    return false;
  }

  bool ok = true;

  while (*str != '\0') {
    if (!uart_avr_write((uint8_t)(*str))) {
      ok = false;
    }

    str++;
  }

  return ok;
}

/**
 * @brief Transmit a null-terminated string directly from Flash (PROGMEM).
 *
 * Reads bytes from Program Flash memory using LPM instruction, streaming
 * directly into the TX buffer with zero SRAM consumption.
 *
 * @param progmem_str Pointer to Flash memory string.
 * @return bool true if all bytes enqueued successfully.
 */
bool uart_avr_write_str_P(const char *progmem_str) {
  if (progmem_str == (void *)0) {
    return false;
  }

  bool ok = true;

  while (1) {
    uint8_t byte = (uint8_t)pgm_read_byte(progmem_str++);

    if (byte == 0) {
      break;
    }

    if (!uart_avr_write(byte)) {
      ok = false;
    }
  }

  return ok;
}

/**
 * @brief Block until all queued TX data is flushed and physical shift register is empty.
 *
 * @return bool true when transmission is completely finished.
 */
bool uart_avr_flush_tx(void) {
  while (!ring_buffer_is_empty(&s_tx_ring) || !(UCSR0A & (1 << UDRE0))) {
    /* Wait for buffer to drain completely */
  }

  return true;
}

/**
 * @brief Check if transmit buffer is empty and hardware transmitter is idle.
 *
 * @return bool true if no transmission is in flight.
 */
bool uart_avr_tx_complete(void) { return (ring_buffer_is_empty(&s_tx_ring) && ((UCSR0A & (1 << UDRE0)) != 0)); }

/**
 * @brief Check if unread bytes are waiting in the receive ring buffer.
 *
 * @param context Unused (provided for icomm.h compliance).
 * @return bool true if at least one byte is available to read.
 */
static bool uart_stream_available(void *context) {
  (void)context;

  return uart_avr_available();
}

/**
 * @brief Read a single byte from the receive ring buffer.
 *
 * @param context Unused (provided for icomm.h compliance).
 * @return uint8_t Next unread byte, or 0 if buffer is empty.
 */
static uint8_t uart_stream_read(void *context) {
  (void)context;

  return uart_avr_read();
}

/**
 * @brief Non-blocking transmit byte over USART0 via TX ring buffer.
 *
 * @param context Unused (provided for icomm.h compliance).
 * @param byte Byte to transmit.
 * @return bool true if enqueued successfully.
 */
static bool uart_stream_write(void *context, uint8_t byte) {
  (void)context;

  return uart_avr_write(byte);
}

/**
 * @brief Transmit a null-terminated string residing in SRAM.
 *
 * @param context Unused (provided for icomm.h compliance).
 * @param str Pointer to null-terminated RAM string.
 * @return bool true if all bytes enqueued successfully.
 */
static bool uart_stream_write_str(void *context, const char *str) {
  (void)context;

  return uart_avr_write_str(str);
}

/**
 * @brief Transmit a null-terminated string directly from Flash (PROGMEM).
 *
 * @param context Unused (provided for icomm.h compliance).
 * @param flash_str Pointer to Flash memory string.
 * @return bool true if all bytes enqueued successfully.
 */
static bool uart_stream_write_str_p(void *context, const char *flash_str) {
  (void)context;

  return uart_avr_write_str_P(flash_str);
}

/**
 * @brief Factory function producing an abstract communication stream handle.
 *
 * Adapts USART0 driver to the icomm.h interface for Dependency Injection
 * into the command parser and telemetry subsystems.
 *
 * @return comm_stream_t Abstract stream interface structure.
 */
comm_stream_t uart_avr_get_stream(void) {
  comm_stream_t stream;
  stream.context = (void *)0;

  stream.available = uart_stream_available;
  stream.read = uart_stream_read;
  stream.write = uart_stream_write;
  stream.write_str = uart_stream_write_str;
  stream.write_str_p = uart_stream_write_str_p;

  return stream;
}
