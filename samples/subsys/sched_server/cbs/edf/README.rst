.. _cbs_sample_edf:

Constant Bandwidth Server (CBS) on EDF
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
   :zephyr-app: samples/subsys/server/cbs/edf
   :host-os: unix
   :board: qemu_riscv32
   :goals: run
   :compact:

To build and run on a physical target (i.e. XIAO ESP32-C3) instead,
run the following:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/server/cbs/edf
   :board: xiao_esp32c3
   :goals: build flash
   :compact:

Sample Output
=============

The example below shows the output for the target xiao_esp32c3.
The value alongside the event log is the budget level, in
hardware cycles. The CBS thread does an underlying conversion
from timeout units passed on ``K_CBS_DEFINE()`` (e.g. ``K_USEC()``)
to ensure units compatibility with ``k_thread_deadline_set()``,
which currently accepts only hardware cycles.

.. code-block:: console

  [job]           j1 on xiao_esp32c3/esp32c3, 27

  [job]           j1 on xiao_esp32c3/esp32c3, 28

  [job]           j1 on xiao_esp32c3/esp32c3, 29
