.. image:: _static/trackedbot_logo.png
   :align: center
   :width: 140px
   :alt: trackedbot logo

trackedbot
==========

**trackedbot** is tracked robot.

`trackedbot <https://en.wikipedia.org/wiki/Category:Tracked_robots>`_ is developed in C code: **100%**.

.. image:: _static/body_platform.png
   :align: center
   :width: 500px
   :alt: trackedbot Mobile Platform

An autonomous tracked mobile robot platform built on the all-metal T101 aluminum chassis, powered by an Uno+WiFi R3 (ATmega328P + ESP8266) dual-processor board, L293D motor driver shield, and modular embedded C firmware adhering to SOLID and Hexagonal architecture principles.

|GitHub issues| |GitHub contributors| |Documentation Status|

.. |GitHub issues| image:: https://img.shields.io/github/issues/dof2bot/trackedbot.svg
   :target: https://github.com/dof2bot/trackedbot/issues

.. |GitHub contributors| image:: https://img.shields.io/github/contributors/dof2bot/trackedbot.svg
   :target: https://github.com/dof2bot/trackedbot/graphs/contributors

.. |Documentation Status| image:: https://readthedocs.org/projects/trackedbot/badge/?version=latest
   :target: https://trackedbot.readthedocs.io/projects/trackedbot/en/latest/?badge=latest

Project Structure
-----------------

The **trackedbot** project is organized into three decoupled layers:

.. toctree::
   :maxdepth: 2
   :caption: System Documentation

   mechanical
   hardware
   software

Source Documentation
--------------------

* **Mechanical Architecture:** `body/README.md <https://github.com/dof2bot/trackedbot/blob/dev/body/README.md>`_
* **Hardware & Electrical Architecture:** `hw/README.md <https://github.com/dof2bot/trackedbot/blob/dev/hw/README.md>`_
* **Software Architecture:** `sw/README.md <https://github.com/dof2bot/trackedbot/blob/dev/sw/README.md>`_
* **Binary Protocol (ICD):** `sw/protocol.md <https://github.com/dof2bot/trackedbot/blob/dev/sw/protocol.md>`_

Copyright and Licence
---------------------

|License: GPL v3| |License: Apache 2.0|

.. |License: GPL v3| image:: https://img.shields.io/badge/License-GPLv3-blue.svg
   :target: https://www.gnu.org/licenses/gpl-3.0

.. |License: Apache 2.0| image:: https://img.shields.io/badge/License-Apache%202.0-blue.svg
   :target: https://opensource.org/licenses/Apache-2.0

Copyright (C) 2020 by `dof2bot.github.io/trackedbot <https://dof2bot.github.io/trackedbot>`_
