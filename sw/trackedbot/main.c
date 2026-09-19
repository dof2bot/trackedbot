/**
 * @file main.c
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
 * @brief Firmware composition root, dependency injection, and non-blocking superloop.
 *
 * Configures hardware peripherals, constructs concrete driver adapters, injects
 * interfaces into domain controllers, and executes a cooperative round-robin
 * event loop with a dual watchdog safety architecture.
 *
 * Firmware System Architecture & Superloop:
 *
 *               +-------------------------------------------+
 *               |              main() Composition           |
 *               |  Dependency Injection & Hardware Config   |
 *               +---------------------+---------------------+
 *                                     |
 *               +---------------------v---------------------+
 *               |             Hardware Drivers              |
 *               |  [Timer1]  [UART0]  [L293D/74HC595] [LED] |
 *               +---------------------+---------------------+
 *                                     | (Ports / Adapters)
 *               +---------------------v---------------------+
 *               |              Domain Core                  |
 *               |  tracked_motion  failsafe  command_parser  |
 *               +---------------------+---------------------+
 *                                     |
 *      ========================= SUPERLOOP =========================
 *     |                                                             |
 *     |  1. Feed Hardware WDT (wdt_reset - 2s limit)                |
 *     |  2. Step Status LED Blink Engine (led_status_update)        |
 *     |  3. Process Incoming Protocol Frames (command_parser)       |
 *     |  4. Periodic 20ms Tick (50 Hz):                             |
 *     |     - Update Software Failsafe Watchdog (failsafe_update)   |
 *     |     - Check Safety Policy (error_handler_is_motion_allowed) |
 *     |     - Step Kinematics Slew Limiter (tracked_motion_update)  |
 *     |                                                             |
 *      =============================================================
 */

#include "command_parser.h"
#include "error_handler.h"
#include "failsafe_watchdog.h"
#include "icomm.h"
#include "imotor.h"
#include "l293d_shield.h"
#include "led_status.h"
#include "telemetry_reporter.h"
#include "timer_avr.h"
#include "tracked_motion.h"
#include "uart_avr.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>

/** Global tracked platform kinematics instance (allocated in .bss). */
static tracked_motion_t s_tracked_motion;

/** Global binary protocol frame parser instance (allocated in .bss). */
static command_parser_t s_parser;

/** Global communication failsafe watchdog instance (allocated in .bss). */
static failsafe_watchdog_t s_failsafe;

/** Global system error handler and health monitor instance (allocated in .bss). */
static error_handler_t s_error_handler;

/**
 * @brief Callback invoked whenever valid command traffic is parsed or executed.
 *
 * Feeds the software failsafe watchdog timer to prevent safety motion timeouts.
 *
 * @param context Pointer to failsafe_watchdog_t instance.
 * @return bool true if watchdog was successfully fed, false otherwise.
 */
static bool on_command_activity(void *context) {
  failsafe_watchdog_t *watchdog = (failsafe_watchdog_t *)context;

  if (watchdog != (void *)0) {
    return (failsafe_feed(watchdog) == MOTION_STATUS_OK);
  }

  return false;
}

/**
 * @brief Main firmware entrypoint.
 *
 * Initializes hardware peripherals, creates software abstractions, registers
 * safety callbacks, enables watchdogs and interrupts, and enters the main superloop.
 *
 * @return int Standard main return code (never returns in embedded context).
 */
int main(void) {
  /* Initialize Error Handler and Diagnostic LED (D13 / PB5). */
  if (!led_status_init()) {
    /* If diagnostic LED fails to init, proceed to init error handler */
  }
  (void)error_handler_init(&s_error_handler);

  /* Initialize Hardware Drivers (Adapters). */
  if (!timer_avr_init()) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_TIMER);
  }

  if (!uart_avr_init(UART_DEFAULT_BAUD_RATE)) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_UART);
  }

  if (l293d_init() != MOTION_STATUS_OK) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_MOTORS);
  }

  /* Obtain Abstract Motor and Stream Handles (Ports / DIP). */
  motor_handle_t left_motor = l293d_get_motor_handle(L293D_CHANNEL_3, false);
  motor_handle_t right_motor = l293d_get_motor_handle(L293D_CHANNEL_4, false);
  comm_stream_t stream = uart_avr_get_stream();

  /* Initialize Domain Logic via Dependency Injection. */
  if (tracked_motion_init(&s_tracked_motion, left_motor, right_motor, (uint8_t)TRACKED_MOTION_SPEED_DEFAULT) !=
      MOTION_STATUS_OK) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_MOTORS);
  }

  if (command_parser_init(&s_parser, &s_tracked_motion, stream) != MOTION_STATUS_OK) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_PARSER);
  }

  if (failsafe_init(&s_failsafe, &s_tracked_motion, (uint32_t)FAILSAFE_DEFAULT_TIMEOUT_MS) != MOTION_STATUS_OK) {
    (void)error_handler_report(&s_error_handler, ERR_CODE_INIT_FAILSAFE);
  }

  (void)command_parser_set_error_handler(&s_parser, &s_error_handler);
  (void)command_parser_set_failsafe(&s_parser, &s_failsafe);
  (void)command_parser_set_activity_cb(&s_parser, on_command_activity, (void *)&s_failsafe);

  /* Enable Hardware Watchdog Timer (2s timeout for hard reset). */
  wdt_enable(WDTO_2S);

  /* Enable Global Interrupts. */
  sei();

  /* Announce initial system status via binary telemetry frame. */
  (void)telemetry_reporter_send_status(&stream, &s_tracked_motion, &s_error_handler);

  /* Get first periodic tick time in milliseconds. */
  uint32_t last_periodic_tick = timer_avr_millis();

  while (1) {
    /* Feed hardware watchdog timer. */
    wdt_reset();

    /* Non-blocking diagnostic LED pattern update driven by system mode. */
    uint32_t now = timer_avr_millis();
    (void)led_status_update(error_handler_get_mode(&s_error_handler), now);

    /* Process incoming line-based commands from WiFi / Serial. */
    (void)command_parser_process(&s_parser);

    /* Periodic safety watchdog & kinematics ramp update every 20ms. */
    if (timer_avr_interval_elapsed(&last_periodic_tick, (uint32_t)FAILSAFE_CHECK_INTERVAL_MS)) {
      (void)failsafe_update(&s_failsafe, (uint32_t)FAILSAFE_CHECK_INTERVAL_MS);

      /* Enforce Safety Policy: Only drive motors if motion is allowed. */
      if (error_handler_is_motion_allowed(&s_error_handler)) {
        (void)tracked_motion_update(&s_tracked_motion, (uint32_t)FAILSAFE_CHECK_INTERVAL_MS);
      } else {
        (void)tracked_motion_stop(&s_tracked_motion);
      }
    }
  }

  return 0;
}
