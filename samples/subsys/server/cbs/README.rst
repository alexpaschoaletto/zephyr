.. _cbs_sample:

Constant Bandwidth Server (CBS) Sample
######################################

Overview
********

A sample that creates a Constant Bandwidth Server (CBS) and pushes a 
simple job to the server queue every two seconds.

Two threads are created - a periodic thread, which runs every two seconds,
and a CBS thread, which runs whatever jobs assigned to it. In this example,
the job simply prints some information on the console and increments a
counter. The periodic thread (i.e. the task) then sets its own deadline at
the beginning of its infinite loop, pushes some jobs to the CBS queue and
finishes its execution, then sleeping for 2 seconds and starting over
afterwards.

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
   :zephyr-app: samples/subsys/server/cbs
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build for another board, change "qemu_riscv32" above to that board's name.


Sample Output
=============

The example below shows the output for the target xiao_esp32c3. The value alongside
the event log is the budget level, in hardware cycles. The CBS thread does an underlying
conversion from timeout units passed on ``K_CBS_DEFINE()`` (e.g. ``K_USEC()``) to ensure units
compatibility with ``k_thread_deadline_set()``, which currently accepts only hardware cycles.

.. code-block:: console

  [job]           j1 on xiao_esp32c3/esp32c3, 27

  [job]           j1 on xiao_esp32c3/esp32c3, 28

  [job]           j1 on xiao_esp32c3/esp32c3, 29

  [00:00:47.069,000] <inf> CBS: cbs_1     J_PUSH  25660       // first job is pushed to the queue
  [00:00:47.069,000] <inf> CBS: cbs_1     B_COND  160000      // condition is met, budget is replenished
  [00:00:47.069,000] <inf> CBS: cbs_1     J_PUSH  160000      // other job is pushed
  [00:00:47.069,000] <inf> CBS: cbs_1     SWT_TO  160000      // CBS thread enters CPU
  [00:00:47.073,000] <inf> CBS: cbs_1     J_COMP  104669      // first job completed
  [00:00:47.077,000] <inf> CBS: cbs_1     J_COMP  25687       // second job completed
  [00:00:47.077,000] <inf> CBS: cbs_1     SWT_AY  25687       // CBS thread leaves CPU

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
