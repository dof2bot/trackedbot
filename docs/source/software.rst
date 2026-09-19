Software Architecture & SOLID Design
=====================================

The **trackedbot** firmware implements a **Hexagonal Software Architecture (Ports & Adapters)** in Embedded C99 for the 8-bit AVR **ATmega328P** microcontroller (16 MHz).

Architectural Principles
------------------------

1. **Dependency Inversion Principle (DIP):** Contracts reside in ``core/interfaces/``. Domain interactors define their port contracts (``imotor.h``, ``icomm.h``, ``motion_types.h``).
2. **Single Responsibility Principle (SRP):** Core domain logic in ``core/`` has zero dependencies on AVR hardware headers (``<avr/io.h>``) or registers. It is 100% portable C99 and testable on native host platforms.
3. **Open/Closed Principle (OCP):** Microcontroller peripherals (Timer0 PWM, 74HC595 shift register, USART0 with interrupt-driven ring buffers, Timer1 CTC) are encapsulated inside ``drivers/`` and implement core contracts.
4. **Composition Root:** The ``main.c`` file is the sole point where concrete drivers are injected into domain interactors and the hardware Watchdog (``wdt_enable(WDTO_2S)``) is managed.

Core Domain Components
----------------------

* **tracked_motion:** Skid-steering kinematics computing differential track velocities.
* **slew_limiter:** Periodic 20 ms acceleration ramp preventing battery voltage sag and L293D inrush current spikes.
* **command_parser:** Robust line-based ASCII parser and binary frame decoder with CRC-8 validation.
* **failsafe_watchdog:** Software watchdog monitoring communication activity and triggering an automatic stop upon link timeout.
* **ring_buffer:** Lock-free Single-Producer Single-Consumer (SPSC) circular queue for asynchronous interrupt-driven UART.

Binary Communication Protocol (ICD)
-----------------------------------

The firmware implements a deterministic binary protocol over UART / Wi-Fi (115200 baud, 8-N-1):

.. code-block:: text

   +--------+--------+--------+--------------------------+--------+
   |  SYNC  | MSG_ID |  LEN   |         PAYLOAD          |  CRC8  |
   | 1 Byte | 1 Byte | 1 Byte |        0..16 Bytes       | 1 Byte |
   +--------+--------+--------+--------------------------+--------+

* **SYNC:** ``0xAA`` frame synchronization delimiter.
* **MSG_ID:** Message identifier (``0x01`` CMD_MOTION, ``0x02`` CMD_SET_SPEED, ``0x03`` CMD_STOP, ``0x04`` CMD_PING, ``0x10`` CMD_GET_STATUS).
* **CRC8:** ATM / SMBus polynomial (:math:`x^8 + x^2 + x + 1`, ``0x07``) protecting integrity.

Detailed Guides
---------------

* `Software Architecture & SOLID Design (sw/README.md) <https://github.com/dof2bot/trackedbot/blob/dev/sw/README.md>`_
* `Binary Protocol Specification (sw/protocol.md) <https://github.com/dof2bot/trackedbot/blob/dev/sw/protocol.md>`_
