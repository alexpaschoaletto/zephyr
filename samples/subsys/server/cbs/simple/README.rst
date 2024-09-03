.. _cbs_sample_simple:

Constant Bandwidth Server (CBS) - Simple
########################################

Overview
********

A sample that creates a Constant Bandwidth Server (CBS) and pushes a 
simple job to the server queue every two seconds. In this example,
the job simply prints some information on the console and increments
a counter. The main thread (i.e. the periodic task) pushes some jobs
to the CBS queue and sleeps for 2 seconds, starting over afterwards.

Event logging
=============

The logging feature is enabled by default, which allows users to see
the key events of a CBS in the console as they happen. The events are:

.. list-table:: Constant Bandwidth Server (CBS) events
   :widths: 10 90
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

The server deadline recalculates whenever the budget is restored - that is, when
either the condition is met (B_COND) or it simply runs out (B_ROUT). In the first
case, the deadline will be set as job arrival instant + CBS period, whereas in the
latter it will be simply postponed by the CBS period.  

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/server/cbs/simple
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build and run on a physical target (i.e. XIAO ESP32-C3) instead,
run the following:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/server/cbs/simple
   :board: xiao_esp32c3
   :goals: build flash
   :compact:

Sample Output
=============

The example below shows the output for the target qemu_riscv32.
The value alongside the event log is the budget level, in
hardware cycles. The CBS thread does an underlying conversion
from timeout units passed on :c:macro:`K_CBS_DEFINE` (e.g. :c:macro:`K_USEC`)
to ensure units compatibility with :c:func:`k_thread_deadline_set`,
which currently accepts only hardware cycles.

.. code-block:: console

  [job]           j1 on qemu_riscv32/qemu_virt_riscv32, 15

  [job]           j1 on qemu_riscv32/qemu_virt_riscv32, 16

  [job]           j1 on qemu_riscv32/qemu_virt_riscv32, 17

  [00:00:12.028,000] <inf> CBS: cbs_1     J_PUSH  43543     // first job is pushed
  [00:00:12.028,000] <inf> CBS: cbs_1     B_COND  100000    // conditiom met, budget replenished
  [00:00:12.028,000] <inf> CBS: cbs_1     J_PUSH  100000    // more jobs pushed
  [00:00:12.028,000] <inf> CBS: cbs_1     J_PUSH  100000
  [00:00:12.028,000] <inf> CBS: cbs_1     SWT_TO  100000    // CBS thread enters CPU to execute
  [00:00:12.031,000] <inf> CBS: cbs_1     J_COMP  68954     // first job completed
  [00:00:12.033,000] <inf> CBS: cbs_1     J_COMP  54372
  [00:00:12.034,000] <inf> CBS: cbs_1     J_COMP  38914     // last job completed
  [00:00:12.034,000] <inf> CBS: cbs_1     SWT_AY  38914     // CBS thread leaves the CPU
