#!/bin/bash
#
# @brief   Setup udev rules and persistent symlink for tracked_bot flashing
# @version 1.0.0
# @date    Fri Sep 18 17:15:00 2026
# @company None, free software to use 2026
# @author  Vladimir Roncevic <elektron.ronca@gmail.com>
#

set -euo pipefail

RULES_FILE="/etc/udev/rules.d/99-tracked-bot.rules"

echo "=== Setting up udev rules for tracked_bot ==="

if [ "$(id -u)" -ne 0 ]; then
    echo "This script requires root privileges to configure /etc/udev/rules.d/"
    echo "Please execute: sudo bash $0"
    exit 1
fi

TARGET_USER="${SUDO_USER:-$USER}"

echo "1. Creating ${RULES_FILE}..."
cat << 'EOF' > "${RULES_FILE}"
# -----------------------------------------------------------------------------
# tracked_bot udev rules
# Grants rw permissions, prevents ModemManager/brltty interference,
# and creates persistent /dev/tracked_bot symlink for make flash.
# -----------------------------------------------------------------------------

# CH340 / CH341 USB-to-UART (Wemos Uno+WiFi / Arduino Uno clone)
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"

# Official Arduino Uno R3 (ATmega16U2)
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0043", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"
SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", ATTRS{idProduct}=="0001", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"
SUBSYSTEM=="tty", ATTRS{idVendor}=="2a03", ATTRS{idProduct}=="0043", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"

# FTDI FT232R USB-to-UART
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"

# Silicon Labs CP210x USB-to-UART
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE="0666", GROUP="dialout", ENV{ID_MM_DEVICE_IGNORE}="1", ENV{BRLTTY_DEVICE_IGNORE}="1", SYMLINK+="tracked_bot"
EOF

chmod 644 "${RULES_FILE}"

if [ -n "${TARGET_USER}" ] && [ "${TARGET_USER}" != "root" ]; then
    echo "2. Ensuring user '${TARGET_USER}' is member of 'dialout' group..."
    if ! id -nG "${TARGET_USER}" | grep -qw "dialout"; then
        usermod -aG dialout "${TARGET_USER}"
        echo "   User '${TARGET_USER}' added to group 'dialout'."
    else
        echo "   User '${TARGET_USER}' is already in group 'dialout'."
    fi
fi

echo "3. Reloading and triggering udev rules..."
udevadm control --reload-rules
udevadm trigger

echo "4. Verifying symlink /dev/tracked_bot..."
if [ -e "/dev/tracked_bot" ]; then
    REAL_PORT=$(readlink -f /dev/tracked_bot)
    echo "   [SUCCESS] /dev/tracked_bot exists -> ${REAL_PORT}"
else
    echo "   [INFO] /dev/tracked_bot not present yet. Re-plug the board USB cable to activate."
fi

echo "=== Setup completed successfully ==="
