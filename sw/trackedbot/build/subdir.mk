# subdir.mk
# Copyright (C) 2026 Vladimir Roncevic <elektron.ronca@gmail.com>
#
# sr14_bot is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# sr14_bot is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program_name. If not, see <http://www.gnu.org/licenses/>.

include tools.mk
include cflags.mk

C_SRCS += \
	../main.c \
	../drivers/shift_reg/shift_reg.c \
	../drivers/l293d_shield/l293d_shield.c \
	../drivers/uart_avr/uart_avr.c \
	../drivers/timer_avr/timer_avr.c \
	../core/tracked_motion/tracked_motion.c \
	../core/slew_limiter/slew_limiter.c \
	../core/command_parser/command_parser.c \
	../core/command_parser/command_dispatcher.c \
	../core/command_parser/command/cmd_motion.c \
	../core/command_parser/command/cmd_speed.c \
	../core/command_parser/command/cmd_stop.c \
	../core/command_parser/command/cmd_ping.c \
	../core/command_parser/command/cmd_status.c \
	../core/command_parser/command/cmd_diag.c \
	../core/command_parser/command/cmd_err_clear.c \
	../core/telemetry_reporter/telemetry_reporter.c \
	../core/crc8/crc8.c \
	../core/failsafe_watchdog/failsafe_watchdog.c \
	../core/ring_buffer/ring_buffer.c \
	../core/error_handler/error_handler.c \
	../drivers/led_status/led_status.c

OBJS += \
	./main.o \
	./shift_reg.o \
	./l293d_shield.o \
	./uart_avr.o \
	./timer_avr.o \
	./tracked_motion.o \
	./slew_limiter.o \
	./command_parser.o \
	./command_dispatcher.o \
	./cmd_motion.o \
	./cmd_speed.o \
	./cmd_stop.o \
	./cmd_ping.o \
	./cmd_status.o \
	./cmd_diag.o \
	./cmd_err_clear.o \
	./telemetry_reporter.o \
	./crc8.o \
	./failsafe_watchdog.o \
	./ring_buffer.o \
	./error_handler.o \
	./led_status.o

C_DEPS += \
	./main.d \
	./shift_reg.d \
	./l293d_shield.d \
	./uart_avr.d \
	./timer_avr.d \
	./tracked_motion.d \
	./slew_limiter.d \
	./command_parser.d \
	./command_dispatcher.d \
	./cmd_motion.d \
	./cmd_speed.d \
	./cmd_stop.d \
	./cmd_ping.d \
	./cmd_status.d \
	./cmd_diag.d \
	./cmd_err_clear.d \
	./telemetry_reporter.d \
	./crc8.d \
	./failsafe_watchdog.d \
	./ring_buffer.d \
	./error_handler.d \
	./led_status.d

%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/command_parser/command/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/shift_reg/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/l293d_shield/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/uart_avr/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/timer_avr/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/tracked_motion/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/slew_limiter/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/command_parser/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/failsafe_watchdog/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/ring_buffer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/error_handler/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../drivers/led_status/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/telemetry_reporter/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../core/crc8/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	$(CC) $(CCFLAGS) "$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

