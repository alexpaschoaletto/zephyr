.. zephyr:code-sample:: edf
   :name: Earliest Deadline First (EDF)

   Implements a basic EDF thread set.

Overview
********

TODO

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/kernel/edf
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board target, replace "qemu_x86" above with it.

Sample Output
=============

TODO

.. code-block:: console

   [producer] sending: 0
   [producer] sending: 1
   [producer] sending: A (urgent)
   [producer] sending: 2
   [producer] sending: 3
   [producer] sending: B (urgent)
   [producer] sending: 4
   [producer] sending: 5
   [producer] sending: C (urgent)
   [consumer] got sequence: CBA012345

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
