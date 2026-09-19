# Main Controller Board - Uno+WiFi R3 (ATmega328P + ESP8266)

Hardware reference, component architecture, pinout, and configuration guide for the **Uno+WiFi R3** hybrid development board (commonly marketed as **TZT Uno WiFi R3**, **RobotDyn Uno+WiFi**, or **WeMos Uno WiFi R3 ATmega328P+ESP8266**) used as the main controller in the **tracked_bot** platform.

This board integrates **both** a **Microchip ATmega328P** primary motion microcontroller and an **Espressif ESP8266EX (32Mb flash)** wireless communications coprocessor, alongside a **CH340G** USB-to-UART bridge on a single standard Arduino Uno R3 footprint.

---

## 📋 Table of Contents

1. [Hardware Overview & Key Components](#-hardware-overview--key-components)
2. [Datasheet References](#-datasheet-references)
3. [Component Specifications](#-component-specifications)
4. [8-Position DIP Switch Configuration](#-8-position-dip-switch-configuration)
5. [Complete Pinout and Signal Routing](#-complete-pinout-and-signal-routing)
6. [Operational Workflows (Programming & Running)](#-operational-workflows-programming--running)
7. [Hardware Interconnection Architecture](#-hardware-interconnection-architecture)

---

## 🔧 Hardware Overview & Key Components

The TZT Uno WiFi R3 replaces a multi-board stack with a dual-processor architecture:

1. **Primary Motion Controller (ATmega328P):**
   - 8-bit AVR RISC microcontroller clocked at 16 MHz.
   - Handles real-time motor control (Timer0 Fast PWM), 74HC595 shift register direction timing, odometry, and differential steering kinematics.
   - Runs deterministic, bare-metal C firmware without RTOS overhead.

2. **Communications Coprocessor (ESP8266EX):**
   - 32-bit Tensilica Xtensa LX106 processor (clocked at 80/160 MHz) with 32 Mbit (4 MB) external SPI flash memory.
   - Provides 802.11 b/g/n Wi-Fi connectivity (TCP/UDP, WebSocket, or HTTP REST server).
   - Manages network packet reception, telemetry broadcasting, and forwards commands to the ATmega328P via hardware USART at 9600 / 115200 baud.

3. **USB-UART Interface (CH340G):**
   - Provides USB Type-B connectivity to a host development PC for firmware flashing and serial monitoring.
   - Multiplexed via an onboard 8-position DIP switch to connect either to the ATmega328P or the ESP8266.

---

## 📚 Datasheet References

Original manufacturer datasheets for all core integrated circuits on this board are preserved in the repository:

- [ATmega328P Datasheet (Microchip 8-bit AVR Microcontroller)](datasheets/atmega328p.pdf)
- [ESP8266EX Datasheet (Espressif 2.4 GHz Wi-Fi SoC)](datasheets/esp8266ex.pdf)
- [CH340 USB to Serial UART Bridge Datasheet](datasheets/ch340.pdf)

---

## ⚡ Component Specifications

| Feature | Primary Controller (ATmega328P) | Wi-Fi Coprocessor (ESP8266EX) |
| :--- | :--- | :--- |
| **Architecture** | 8-bit AVR RISC | 32-bit Tensilica Xtensa L106 |
| **Clock Frequency** | 16 MHz (external crystal) | 80 MHz / 160 MHz |
| **Operating Voltage** | 5.0 V DC | 3.3 V DC (onboard LDO regulator) |
| **Flash Memory** | 32 KB (0.5 KB used by bootloader) | 4 MB (32 Mbit SPI Flash) |
| **SRAM** | 2 KB internal SRAM | ~50 KB usable RAM |
| **EEPROM** | 1 KB internal EEPROM | None (emulated in SPI flash sector) |
| **Timers** | 2x 8-bit (Timer0, Timer2), 1x 16-bit (Timer1) | System timers, Software PWM |
| **Hardware Serial (UART)** | 1x USART (`PD0/RX`, `PD1/TX`) | 1x Full UART (UART0), 1x TX-only (UART1) |
| **I/O Logic Levels** | 5 V TTL/CMOS | 3.3 V LVTTL (tolerant on UART via dividers) |

---

## 🎛️ 8-Position DIP Switch Configuration

The onboard 8-position DIP switch physically routes the UART TX/RX lines between the CH340G USB converter, the ATmega328P microcontroller, and the ESP8266 Wi-Fi SoC:

| Mode / Functionality | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Mode 1: Flash ATmega328P (`make flash`)** | OFF | OFF | **ON** | **ON** | OFF | OFF | OFF | N/U |
| **Mode 2: Normal Robot Operation (ATmega328P ↔ ESP8266)** | **ON** | **ON** | OFF | OFF | OFF | OFF | OFF | N/U |
| **Mode 3: Flash ESP8266 Firmware** | OFF | OFF | OFF | OFF | **ON** | **ON** | **ON** | N/U |
| **Mode 4: ESP8266 Serial Monitor Debug** | OFF | OFF | OFF | OFF | **ON** | **ON** | OFF | N/U |
| **Mode 5: All Modules Disconnected / Isolated** | OFF | OFF | OFF | OFF | OFF | OFF | OFF | N/U |

> [!NOTE]
> - Switch **SW8** is unused on this board revision (labeled *No Use / N/U*).
> - Switch **SW7** controls ESP8266 `GPIO0` boot mode: `ON` pulls `GPIO0` LOW into UART download/flashing mode; `OFF` pulls `GPIO0` HIGH for normal flash boot.

---

## 📌 Complete Pinout and Signal Routing

The headers on the TZT Uno WiFi R3 follow the standard Arduino Uno R3 footprint. The table below details the signal routing, onboard connections, and usage within `tracked_bot`:

### Digital Pins (D0 - D13)

| Pin | AVR Port | Onboard Connection | Role in `tracked_bot` |
| :--- | :--- | :--- | :--- |
| **D0 (RX)** | `PD0` | ATmega328P USART RX (DIP SW1/SW4) | **USART0 RX:** Serial command stream from ESP8266 |
| **D1 (TX)** | `PD1` | ATmega328P USART TX (DIP SW2/SW3) | **USART0 TX:** Serial telemetry stream to ESP8266 |
| **D2** | `PD2` | Header pin | **FREE GPIO:** External Interrupt 0 (`INT0`), sensors, encoders |
| **D3** | `PD3` | Header pin / L293D `PWM2B` | Reserved (Timer2 `OC2B` for Motor 2 PWM on shield) |
| **D4** | `PD4` | Header pin / L293D `DIR_CLK` | **74HC595 Shift Clock:** Clock line for direction register |
| **D5** | `PD5` | Header pin / L293D `PWM0B` | **Right Track PWM Speed:** Timer0 `OC0B` Fast PWM |
| **D6** | `PD6` | Header pin / L293D `PWM0A` | **Left Track PWM Speed:** Timer0 `OC0A` Fast PWM |
| **D7** | `PD7` | Header pin / L293D `DIR_EN` | **74HC595 Output Enable:** Active-low output buffer enable (`~OE`) |
| **D8** | `PB0` | Header pin / L293D `DIR_SER` | **74HC595 Serial Data:** Direction bit data line (`SER`) |
| **D9** | `PB1` | Header pin / L293D `PWM1A` | Reserved for Pan-Tilt Servo 2 (Timer1 `OC1A`) |
| **D10** | `PB2` | Header pin / L293D `PWM1B` | Reserved for Pan-Tilt Servo 1 (Timer1 `OC1B`) |
| **D11** | `PB3` | Header pin / L293D `PWM2A` | Reserved (Timer2 `OC2A` for Motor 1 PWM on shield) |
| **D12** | `PB4` | Header pin / L293D `DIR_LATCH` | **74HC595 Storage Latch:** Latches direction bits to outputs (`RCLK`) |
| **D13** | `PB5` | Header pin / Onboard SCK LED | **FREE GPIO:** SPI SCK / Built-in indicator LED |

### Analog Pins (A0 - A5 / D14 - D19)

All analog pins are uncommitted by the L293D motor shield and are available on the top header breakout:

| Pin | AVR Port | Alternative Functions | Dedicated Purpose in `tracked_bot` |
| :--- | :--- | :--- | :--- |
| **A0** | `PC0` | ADC0 / PCINT8 | **Battery Voltage Sense:** 10:1 resistor divider to ADC0 |
| **A1** | `PC1` | ADC1 / PCINT9 | **HC-SR04 Ultrasonic Trigger:** Pulse generator |
| **A2** | `PC2` | ADC2 / PCINT10 | **HC-SR04 Ultrasonic Echo:** Pulse duration measurement |
| **A3** | `PC3` | ADC3 / PCINT11 | **Auxiliary GPIO / Analog Input** |
| **A4** | `PC4` | ADC4 / I2C SDA | **I2C Bus SDA:** IMU gyroscope/accelerometer (MPU-6050) / OLED |
| **A5** | `PC5` | ADC5 / I2C SCL | **I2C Bus SCL:** IMU gyroscope/accelerometer (MPU-6050) / OLED |

---

## 🚀 Operational Workflows (Programming & Running)

### Workflow 1: Flashing ATmega328P Firmware (tracked_bot)

To flash the bare-metal AVR C hex firmware onto the ATmega328P:

1. Disconnect or power down the robot's external motor power supply.
2. Set the 8-position DIP switch to **Mode 1**:
   - **SW3 = ON**, **SW4 = ON**
   - **SW1, SW2, SW5, SW6, SW7, SW8 = OFF**
3. Connect the board to your development machine using a USB Type-B cable.
4. Execute the build and flash target:
   ```bash
   make -C sw/trackedbot/build flash PORT=/dev/ttyUSB0
   ```
5. `avrdude` will synchronize with the optiboot / STK500v1 bootloader over `/dev/ttyUSB0` at 115200 baud and flash the firmware image.

---

### Workflow 2: Normal Robot Operation (Wireless Control)

After flashing the firmware:

1. Disconnect the USB cable or switch the DIP switch with power off.
2. Set the 8-position DIP switch to **Mode 2**:
   - **SW1 = ON**, **SW2 = ON**
   - **SW3, SW4, SW5, SW6, SW7, SW8 = OFF**
3. Mount the [L293D Motor Shield](motor_shield.md) onto the board headers.
4. Connect external battery power according to the [power_supply.md](power_supply.md) guide.
5. In this mode, ATmega328P pins `D0 (RX)` and `D1 (TX)` communicate directly with the ESP8266 UART at 9600 baud. The ESP8266 listens to Wi-Fi control packets and streams motion commands to the AVR core.

---

### Workflow 3: Flashing ESP8266 Wi-Fi Firmware

When flashing an updated Wi-Fi firmware image or Arduino ESP8266 sketch:

1. Set the 8-position DIP switch to **Mode 3**:
   - **SW5 = ON**, **SW6 = ON**, **SW7 = ON**
   - **SW1, SW2, SW3, SW4, SW8 = OFF**
2. Connect USB to PC and run `esptool.py` (or Arduino IDE with board set to *Generic ESP8266 Module*).
3. Upon completion, switch to **Mode 4** (turn SW7 OFF) to monitor debug output, or **Mode 2** to reconnect to ATmega328P.

---

## ⚡ Hardware Interconnection Architecture

```
                 +------------------------------------------------+
                 |              TZT Uno WiFi R3 Main Board        |
                 |                                                |
                 |   +------------+           +---------------+   |
 [USB to PC] ======> |   CH340G   |           |    ESP8266    |   |
                 |   |  USB-UART  |           |   Wi-Fi SoC   | <===> [Wi-Fi 802.11 b/g/n]
                 |   +-----+------+           +-------+-------+   |
                 |         |                          |           |
                 |         +----[ 8-Position DIP ]----+           |
                 |                      |                         |
                 |            (Serial D0/RX, D1/TX)               |
                 |                      |                         |
                 |              +-------v-------+                 |
                 |              |  ATmega328P   |                 |
                 |              | Primary MCU   |                 |
                 |              +-------+-------+                 |
                 +----------------------|-------------------------+
                                        |
                 (PWM D5/D6, Shift Reg D4/D7/D8/D12, ADC A0..A5)
                                        |
                 +----------------------v-------------------------+
                 |        L293D Motor Driver Shield               |
                 |  (IC1, IC2 H-Bridges + 74HC595 Shift Register) |
                 +------------------------------------------------+
```

For the motor shield driver circuits and register assignments, refer to [motor_shield.md](motor_shield.md). For power rail wiring and battery safety, refer to [power_supply.md](power_supply.md).

---

## 🏷️ Board Variants & Commercial Branding (TZT, RobotDyn, WeMos)

The hybrid Uno+WiFi R3 board is manufactured and distributed under several commercial brand names across various suppliers:

- **TZT Uno WiFi R3 (ATmega328P + ESP8266, 32Mb Flash)**
- **RobotDyn Uno+WiFi R3 (ATmega328P + ESP8266)**
- **WeMos Uno R3 WiFi / Geekcreit Uno WiFi (ATmega328P + ESP8266)**

### Important Clarification:
These are **identical physical and electrical implementations** of the same dual-processor architecture:
1. **Both Chips are Present:** Every variant includes both the 8-bit **Microchip ATmega328P** (running at 5.0 V) and the 32-bit **Espressif ESP8266EX** (running at 3.3 V with 32 Mbit / 4 MB SPI flash) alongside the **CH340G** USB bridge.
2. **Standard 8-Position DIP Switch:** All variants feature the same 8-position DIP switch controlling serial multiplexing between the USB interface, AVR core, and ESP8266 SoC.
3. **100% Shield Compatibility:** Because the physical Uno header pins (D0–D13, A0–A5) are wired directly to the ATmega328P, standard Arduino Uno shields (including the [L293D Motor Shield](motor_shield.md)) plug directly onto the board with full 5V TTL logic compatibility and exact Timer0/Timer2 hardware PWM pinout.
4. **Distinction from Standalone ESP8266 Boards:** This dual-SoC board should not be confused with standalone ESP8266 boards (such as the legacy *Wemos D1 R2* which contained only an ESP8266 without an ATmega328P). On this board, the ATmega328P runs the repository's native real-time C firmware ([`sw/trackedbot`](../sw/trackedbot/)), while the ESP8266 handles Wi-Fi networking and streams commands over hardware UART.


