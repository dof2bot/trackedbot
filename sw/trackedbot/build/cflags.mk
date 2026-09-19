# cflags.mk
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

INCLUDES := \
	-I.. \
	-I../core/interfaces \
	-I../core/tracked_motion \
	-I../core/slew_limiter \
	-I../core/command_parser \
	-I../core/command_parser/command \
	-I../core/failsafe_watchdog \
	-I../core/ring_buffer \
	-I../drivers/l293d_shield \
	-I../drivers/uart_avr \
	-I../drivers/shift_reg \
	-I../drivers/timer_avr \
	-I../core/error_handler \
	-I../core/telemetry_reporter \
	-I../core/crc8 \
	-I../drivers/led_status

CFLAGS := \
	-Wl,-Map,tracked_bot.map \
	-mmcu=atmega328p

CCFLAGS := \
	-Wall \
	-g2 \
	-O2 \
	-fpack-struct \
	-fshort-enums \
	-ffunction-sections \
	-fdata-sections \
	-std=gnu99 \
	-funsigned-char \
	-funsigned-bitfields \
	-mmcu=atmega328p \
	-DF_CPU=16000000UL \
	$(INCLUDES) \
	-MMD \
	-MP \
	-MF
