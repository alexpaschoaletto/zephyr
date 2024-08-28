.. _cbs_sample:

Constant Bandwidth Server (CBS) Sample
###########

Overview
********

A sample that creates a Constant Bandwidth Server (CBS) and pushes a 
simple job to the server queue every two seconds.

Event logging
=============

The logging feature is enabled by default, which allows users to see
the key events of a CBS in the console as they happen. The events are:

.. list-table:: Constant Bandwidth Server (CBS) events
   :widths: 10 65
   :header-rows: 1

   * - Event
     - Description
   * - J_PUSH
     - a job has been pushed to the CBS job queue.
   * - J_COMP
     - a job has been completed.
   * - B_COND
     - the CBS condition for replenishing the budget was met.
   * - B_ROUT
     - the budget ran out mid-execution and was replenished.
   * - SWT_TO
     - the CBS thread entered the CPU to execute a received job.
   * - SWT_AY
     - the CBS thread left the CPU due to preemption or ending job execution.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/server/cbs
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build for another board, change "qemu_riscv32" above to that board's name.


Sample Output
=============

.. code-block:: console

    [00:00:05.040,000] <inf> CBS: cbs_1     SWT_TO  200000         // switch to
    [00:00:05.050,000] <inf> CBS: cbs_1     B_STAT  21493
    [00:00:05.060,000] <inf> CBS: cbs_1     B_ROUT  200000
    [00:00:05.060,000] <inf> CBS: cbs_1     B_STAT  191045
    [00:00:05.060,000] <inf> CBS: cbs_1     SWT_AY  191045         // switch away

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
