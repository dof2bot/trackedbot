<img align="right" src="docs/trackedbot_logo.png" alt="trackedbot logo" width="25%"/>

<!-- START doctoc -->
**Table of Contents**

- [Project Structure](#project-structure)
- [Documentation](#documentation)
- [Copyright and Licence](#copyright-and-licence)
<!-- END doctoc -->

An autonomous tracked mobile robot platform built on the all-metal T101 aluminum chassis, powered by an Uno+WiFi R3 (ATmega328P + ESP8266) dual-processor board, L293D motor driver shield, and modular embedded C firmware adhering to SOLID and Hexagonal architecture principles.

![trackedbot Mobile Platform](body/images/body_platform.png)


### PROJECT STRUCTURE

The **trackedbot** project is organized into three decoupled layers:

- **[body/](body/README.md)** – Mechanical chassis, physical specs, track sizing, and assembly guide.
- **[hw/](hw/README.md)** – Hardware schematics, Uno+WiFi R3, L293D shield, master pinout, and power distribution.
- **[sw/](sw/README.md)** – Embedded C firmware architecture, SOLID design, ports & adapters, and drivers.

```mermaid
flowchart TD
    CLIENT["Host Client / Controller<br/>(CLI, Web GUI, Autonomous Navigator)"]

    subgraph COMM ["Communication Medium"]
        direction LR
        UART["Serial UART<br/>(115200 Baud 8-N-1)"]
        WIFI["ESP8266 Wi-Fi<br/>(Transparent Bridge)"]
    end

    subgraph SW ["Software Layer (sw/ - Embedded C Firmware)"]
        direction TB
        PARSER["Binary Protocol Parser<br/>(5-State Resilient FSM)"]
        CMD_DISPATCH["Command Dispatcher<br/>(Command Pattern)"]
        KINEMATICS["Kinematics Engine<br/>(Differential Slew Rate & Mixing)"]
        HAL["Hardware Abstraction Layer<br/>(GPIO, PWM, ShiftReg, ADC)"]
        PARSER --> CMD_DISPATCH --> KINEMATICS --> HAL
    end

    subgraph HW ["Hardware Layer (hw/ - Electronics & Power)"]
        direction TB
        MCU["Uno+WiFi R3 Board<br/>(ATmega328P @ 16 MHz)"]
        SHIELD["L293D Motor Shield<br/>(Dual H-Bridge + 74HC595)"]
        POWER["Dual Power Supply Rails<br/>(5V Logic + 7.4V Motor Power)"]
        SENSE["Battery Voltage Sensing<br/>(ADC Pin A0 Voltage Divider)"]
    end

    subgraph BODY ["Body Layer (body/ - Mechanical Chassis)"]
        direction TB
        CHASSIS["T101 All-Metal Chassis<br/>(Aircraft-Grade 6061 Aluminum)"]
        TRACKS["Differential Track Belts<br/>(Modular Engineering POM Links)"]
        MOTORS["Twin 9V DC Metal Gearmotors<br/>(Left Track M3 / Right Track M1)"]
        CHASSIS --- TRACKS --- MOTORS
    end

    CLIENT --> COMM
    COMM --> SW
    SW --> HW
    HW --> BODY

    style CLIENT fill:#e0f2f1,stroke:#00897b,stroke-width:2px
    style COMM fill:#e8eaf6,stroke:#3f51b5,stroke-width:2px
    style SW fill:#e1f5fe,stroke:#0288d1,stroke-width:2px
    style HW fill:#fff3e0,stroke:#f57c00,stroke-width:2px
    style BODY fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px
```


### DOCS

[![Documentation Status](https://readthedocs.org/projects/trackedbot/badge/?version=latest)](https://trackedbot.readthedocs.io/en/latest/?badge=latest)

More documentation and info at:
* **Mechanical Architecture:** [body/README.md](body/README.md)
* **Hardware & Electrical Architecture:** [hw/README.md](hw/README.md)
* **Software Architecture:** [sw/README.md](sw/README.md)
* [https://trackedbot.readthedocs.io/en/latest/](https://trackedbot.readthedocs.io/en/latest/)

### COPYRIGHT AND LICENCE

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

Copyright (C) 2020 - 2026 by [dof2bot.github.io/trackedbot](https://dof2bot.github.io/trackedbot)
