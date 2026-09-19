Hardware & Electrical Architecture
==================================

The **trackedbot** electrical hardware is built on a modular dual-processor computing stack using the standard Arduino Uno R3 mechanical and electrical footprint.

Hardware Stack Overview
-----------------------

.. list-table::
   :widths: 25 25 25 25
   :header-rows: 1

   * - Subsystem
     - Board Model
     - Function
     - Reference
   * - **Main Controller**
     - **Uno+WiFi R3**
     - ATmega328P (16 MHz) + ESP8266 (80 MHz)
     - `main_board.md <https://github.com/dof2bot/trackedbot/blob/dev/hw/main_board.md>`_
   * - **Motor Driver**
     - **L293D Shield**
     - Dual L293D H-bridges + 74HC595 shift register
     - `motor_shield.md <https://github.com/dof2bot/trackedbot/blob/dev/hw/motor_shield.md>`_
   * - **Power Supply**
     - **Dual Supply Rails**
     - Isolated motor rail (2S Li-ion) & logic rail
     - `power_supply.md <https://github.com/dof2bot/trackedbot/blob/dev/hw/power_supply.md>`_
   * - **Locomotion**
     - **T101 Platform**
     - 6061 Aluminum chassis + DC gearmotors
     - :doc:`mechanical`

Main Controller: Uno+WiFi R3
----------------------------

The controller board integrates an **ATmega328P** AVR microcontroller, an **ESP8266EX** Wi-Fi coprocessor, and a **CH340G** USB-UART bridge. An onboard 8-position DIP switch routes the serial UART lines:

.. list-table::
   :widths: 15 10 10 10 10 10 10 10 15
   :header-rows: 1

   * - Mode
     - SW1
     - SW2
     - SW3
     - SW4
     - SW5
     - SW6
     - SW7
     - Description
   * - **Flash ATmega328P**
     - OFF
     - OFF
     - ON
     - ON
     - OFF
     - OFF
     - OFF
     - USB connected to ATmega328P (make flash)
   * - **Flash ESP8266**
     - OFF
     - OFF
     - OFF
     - OFF
     - ON
     - ON
     - ON
     - USB connected to ESP8266 (BOOT mode)
   * - **Run / Wi-Fi Bridge**
     - ON
     - ON
     - OFF
     - OFF
     - OFF
     - OFF
     - OFF
     - ATmega328P connected to ESP8266 via UART

Motor Driver Shield: L293D & 74HC595
------------------------------------

The motor driver shield utilizes:

* **Dual L293D Quadruple Half-H Drivers:** Channels M3 and M4 provide bidirectional H-bridge control for the left and right tracks.
* **74HC595 8-bit Shift Register:** Serial-to-parallel direction multiplexer controlled via 4 pins (D4 clock, D7 enable, D8 data, D12 latch).
* **Timer0 Fast PWM:** D5 (OC0B, ~976 Hz) drives M3 (Left Track speed); D6 (OC0A, ~976 Hz) drives M4 (Right Track speed).

Power Distribution & Isolation
------------------------------

* **Logic Rail (5V):** Powered via USB or clean 5V step-down buck converter.
* **Motor Rail (EXT_PWR):** Powered via 2S Li-ion / LiPo pack (7.4 V – 8.4 V) through the shield's blue screw terminal.
* **PWR Jumper Safety Rule:** The `PWR` jumper on the L293D shield **must be removed** when using external motor battery power to isolate noisy motor inductances from the delicate MCU logic.
* **EMI Suppression:** 100 nF ceramic capacitors soldered across DC motor brush terminals suppress inductive switching noise.

Detailed Guides
---------------

* `Hardware Architecture Master Guide (hw/README.md) <https://github.com/dof2bot/trackedbot/blob/dev/hw/README.md>`_
* `Main Controller & DIP Switches (hw/main_board.md) <https://github.com/dof2bot/trackedbot/blob/dev/hw/main_board.md>`_
* `L293D Motor Shield Registers (hw/motor_shield.md) <https://github.com/dof2bot/trackedbot/blob/dev/hw/motor_shield.md>`_
* `Power Supply & Battery Calculations (hw/power_supply.md) <https://github.com/dof2bot/trackedbot/blob/dev/hw/power_supply.md>`_
