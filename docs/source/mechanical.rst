Mechanical Platform & Chassis
==============================

The mechanical structure of **trackedbot** is built around the **T101 Mini Tracked Tank Chassis**. It features a lightweight, high-strength 6061 aluminum alloy body, dual modular continuous caterpillar tracks, and two independent DC metal gearmotors for differential skid-steering locomotion.

.. image:: _static/body_platform.png
   :align: center
   :width: 480px
   :alt: T101 Tracked Platform

Platform Features & Architecture
--------------------------------

* **Monolithic 6061 Aluminum Chassis:** Precision CNC-bent 2.0 mm sandblasted and anodized aluminum frame resisting torsional strain during high-torque skid turns.
* **Top Equipment Deck:** Equipped with a central cable pass-through hole (:math:`\varnothing \approx 18\text{ mm}`) and four longitudinal adjustment slots for sensor and bracket mounting.
* **Electronics Standoff Pattern:** Pre-drilled M3 mounting holes matching standard Arduino Uno R3 / Mega 2560 standoff dimensions (:math:`53.3\text{ mm} \times 27.9\text{ mm}`).
* **Toothed Drive Sprockets (Rear):** CNC machined aluminum sprockets keyed to motor D-shafts with M3 set-screws.
* **Bearing Idler Wheels (Front):** Aluminum wheels with dual miniature ball bearings and a 5-screw outer retaining ring for track alignment.
* **Continuous Caterpillar Tracks:** Modular engineering plastic (POM) interlocking links with exterior traction ribs and inner guide horns.
* **Lower Equipment Bay:** Houses dual 25 mm DC gearmotors and the battery pack (2S Li-ion / LiPo) beneath the deck, ensuring a low center of gravity.

T101 Chassis Specifications
---------------------------

.. list-table::
   :widths: 30 35 35
   :header-rows: 1

   * - Parameter
     - Specification
     - Notes
   * - **Chassis Model**
     - **T101 Mini Tracked Platform**
     - Modular all-metal construction
   * - **Frame Material**
     - **6061 Aluminum Alloy**
     - 2.0 mm thickness, anodized
   * - **Track Material**
     - **Engineering Plastic (POM)**
     - Interlocking links with steel hinge pins
   * - **Dimensions (L x W x H)**
     - **~190 mm x 165 mm x 60 mm**
     - Compact desktop footprint
   * - **Track Width**
     - **~40 mm**
     - Large contact patch distributing ground pressure
   * - **Track Gauge (W)**
     - **~125 mm**
     - Centerline distance between tracks
   * - **Ground Clearance**
     - **~18 mm**
     - Center underbody clearance
   * - **Unladen Weight**
     - **~530 g**
     - Frame, wheels, motors, tracks
   * - **Maximum Payload**
     - **~2.0 kg – 3.0 kg**
     - Capacity for battery and electronics
   * - **Locomotion Mode**
     - **Differential Skid-Steering**
     - Zero-radius in-place spin (:math:`R = 0`)

Skid-Steering Kinematics
------------------------

Differential tracked locomotion relies on differing track velocities (:math:`v_L` and :math:`v_R`) to steer:

.. math::

   v = \frac{v_R + v_L}{2}, \quad \omega = \frac{v_R - v_L}{W}

* **Straight Drive:** :math:`v_L = v_R` (Equal PWM duty cycles).
* **Pivot Turn:** :math:`v_L = 0` or :math:`v_R = 0` (Robot pivots around the stationary track; radius :math:`R = W/2`).
* **Zero-Radius Spin:** :math:`v_L = -v_R` (Robot spins about its geometric center with radius :math:`R = 0`).

Detailed Guides
---------------

For the complete mechanical documentation and step-by-step construction guide:

* `Mechanical Architecture Overview (body/README.md) <https://github.com/dof2bot/trackedbot/blob/dev/body/README.md>`_
* `Step-by-Step Assembly Guide (body/assembly_guide.md) <https://github.com/dof2bot/trackedbot/blob/dev/body/assembly_guide.md>`_
