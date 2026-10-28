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
    CLIENT["<b>Host Client / Controller</b><br/>CLI, Web GUI, Autonomous Navigator"]

    subgraph COMM ["<b>Communication Medium</b>"]
        direction LR
        UART["<b>Serial UART</b><br/>115200 Baud 8-N-1"]
        WIFI["<b>ESP8266 Wi-Fi</b><br/>Transparent Bridge"]
    end

    subgraph SW ["<b>Software Layer (sw/)</b>"]
        direction TB
        PARSER["<b>Binary Protocol Parser</b><br/>5-State Resilient FSM"]
        CMD_DISPATCH["<b>Command Dispatcher</b><br/>Command Pattern"]
        KINEMATICS["<b>Kinematics Engine</b><br/>Differential Slew & Mixing"]
        HAL["<b>Hardware Abstraction Layer</b><br/>GPIO, PWM, ShiftReg, ADC"]
        PARSER --> CMD_DISPATCH --> KINEMATICS --> HAL
    end

    subgraph HW ["<b>Hardware Layer (hw/)</b>"]
        direction TB
        MCU["<b>Uno+WiFi R3 Board</b><br/>ATmega328P @ 16 MHz"]
        SHIELD["<b>L293D Motor Shield</b><br/>Dual H-Bridge + 74HC595"]
        POWER["<b>Dual Power Supply Rails</b><br/>5V Logic + 7.4V Motor Power"]
        SENSE["<b>Battery Voltage Sensing</b><br/>ADC Pin A0 Voltage Divider"]
    end

    subgraph BODY ["<b>Body Layer (body/)</b>"]
        direction TB
        CHASSIS["<b>T101 Aluminum Chassis</b><br/>Aircraft-Grade 6061 Aluminum"]
        TRACKS["<b>Differential Track Belts</b><br/>Modular POM Tread Links"]
        MOTORS["<b>Twin 9V DC Gearmotors</b><br/>Left Track M3 / Right Track M1"]
        CHASSIS --- TRACKS --- MOTORS
    end

    CLIENT --> COMM
    COMM --> SW
    SW --> HW
    HW --> BODY

    classDef clientStyle fill:#0f766e,stroke:#14b8a6,stroke-width:2px,color:#ffffff;
    classDef commStyle fill:#312e81,stroke:#6366f1,stroke-width:2px,color:#ffffff;
    classDef swStyle fill:#0369a1,stroke:#38bdf8,stroke-width:2px,color:#ffffff;
    classDef hwStyle fill:#c2410c,stroke:#fb923c,stroke-width:2px,color:#ffffff;
    classDef bodyStyle fill:#701a75,stroke:#e879f9,stroke-width:2px,color:#ffffff;

    class CLIENT clientStyle;
    class UART,WIFI commStyle;
    class PARSER,CMD_DISPATCH,KINEMATICS,HAL swStyle;
    class MCU,SHIELD,POWER,SENSE hwStyle;
    class CHASSIS,TRACKS,MOTORS bodyStyle;

    style COMM fill:none,stroke:#6366f1,stroke-width:2px,stroke-dasharray: 4 4
    style SW fill:none,stroke:#38bdf8,stroke-width:2px,stroke-dasharray: 4 4
    style HW fill:none,stroke:#fb923c,stroke-width:2px,stroke-dasharray: 4 4
    style BODY fill:none,stroke:#e879f9,stroke-width:2px,stroke-dasharray: 4 4
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
