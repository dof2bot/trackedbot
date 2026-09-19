# Makefile
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

include sources.mk
include subdir.mk
include objects.mk
include cflags.mk
include odflags.mk
include ocflags.mk
include csflags.mk
include tools.mk

ifneq ($(MAKECMDGOALS),clean)
	ifneq ($(strip $(I_DEPS)),)
		-include $(I_DEPS)
	endif
	ifneq ($(strip $(ASM_DEPS)),)
		-include $(ASM_DEPS)
	endif
	ifneq ($(strip $(M_DEPS)),)
		-include $(M_DEPS)
	endif
	ifneq ($(strip $(MI_DEPS)),)
		-include $(MI_DEPS)
	endif
	ifneq ($(strip $(S_DEPS)),)
		-include $(S_DEPS)
	endif
	ifneq ($(strip $(S_UPPER_DEPS)),)
		-include $(S_UPPER_DEPS)
	endif
	ifneq ($(strip $(C_DEPS)),)
		-include $(C_DEPS)
	endif
endif

LSS += \
	tracked_bot.lss

FLASH_IMAGE += \
	tracked_bot.hex

SIZEDUMMY += \
	sizedummy

CLEAN_LIST += \
	$(FLASH_IMAGE) \
	$(ELFS) \
	$(I_DEPS) \
	$(OBJS) \
	$(ASM_DEPS) \
	$(M_DEPS) \
	$(MI_DEPS) \
	$(S_DEPS) \
	$(SIZEDUMMY) \
	$(S_UPPER_DEPS) \
	$(LSS) \
	$(C_DEPS) \
	*.i \
	*.s \
	tracked_bot.map \
	tracked_bot.elf

PORT ?= $(shell test -e /dev/tracked_bot && echo /dev/tracked_bot || echo /dev/ttyUSB0)
BAUD ?= 115200

all: tracked_bot.elf secondary-outputs

tracked_bot.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: AVR C Linker'
	$(CC) $(CFLAGS) -o "tracked_bot.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

tracked_bot.lss: tracked_bot.elf
	@echo 'Invoking: AVR Create Extended Listing'
	$(OD) $(ODFLAGS) tracked_bot.elf > "tracked_bot.lss"
	@echo 'Finished building: $@'
	@echo ' '

tracked_bot.hex: tracked_bot.elf
	@echo 'Create Flash image (ihex format)'
	$(OC) $(OCFLAGS) -O ihex tracked_bot.elf "tracked_bot.hex"
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: tracked_bot.elf
	@echo 'Invoking: Print Size'
	$(CS) $(CSFLAGS) tracked_bot.elf
	@echo 'Finished building: $@'
	@echo ' '

flash: tracked_bot.hex
	@echo 'Flashing tracked_bot.hex to Arduino on $(PORT) at $(BAUD) baud...'
	avrdude -v -p atmega328p -c arduino -P $(PORT) -b $(BAUD) -D -U flash:w:tracked_bot.hex:i

clean:
	@for item in $(CLEAN_LIST);do echo "$(RM) $$item";$(RM) $$item;done
	@echo ' '

secondary-outputs: $(LSS) $(FLASH_IMAGE) $(SIZEDUMMY)

.PHONY: all clean secondary-outputs flash
