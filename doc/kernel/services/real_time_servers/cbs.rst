.. _constant_bandwidth_server_v1:

Constant Bandwidth Server (CBS)
###############################

A :dfn:`CBS` is a real-time server meant to allow aperiodic,
non-real-time tasks (i.e. jobs) to be executed within a periodic
real-time taskset running under a dynamic sheduling environment
such as EDF.

.. contents::
    :local:
    :depth: 1

Introduction
************

The theoretical analysis of real-time systems often deals with a known
set of tasks, each with an estimated **Worst-Case Execution Time (WCET)**, 
a given **period** between executions and a **deadline** of which the task
must have executed completely before reaching it. However, many real-time
control applications are often less predictable than that, featuring
tasks that are *event-driven* instead of just *time-driven* and sometimes
even with no defined worst-case execution times or deadlines. Examples of a
*periodic* task can be a sensor reader or a motor driver, while an
*aperiodic* task could be one connected to a *human interface device (HID)*.

This lack of known execution, period or deadline constraints make these
tasks hard to include on a regular schedulability analysis, where such
constraints are precisely what differentiates the many algorithms altogether.
For example, static algorithms such as Rate Monotonic (RM) depend on the period
of each task to determine which has the highest priority, while dynamic
algorithms such as Earliest Deadline First (EDF) make the decision based on
which task has the earliest absolute deadline within a taskset. The relation
between a task period (or deadline, in some cases) and its worst-case execution
time is also a fundamental aspect of the analysis, as it helps understanding
the :dfn:`Utilization Factor` of each task and thus know if the system will
be able to execute all tasks assigned to it.

One of the proposed solutions for dealing with a hybrid taskset (i.e. periodic
and aperiodic tasks) is the use of :dfn:`real-time servers`, which are periodic
tasks that act as a wrapper for aperiodic ones and which sole purpose is to
execute them whenever possible (hence the name "servers"). The existance of
servers then enables aperiodicvtasks to be executed in an environment with
known constraints such as a deadline, a period and/or a maximum execution time
within a period, effectively adding some predictability to them and enabling a
more robust schedulability analysis to be performed.


The Constant Bandwidth Server
*****************************

This kind of server is meant to run alongside a deadline-oriented algorithm
such as EDF, which is supported out-of-the-box by Zephyr. It consists of
three base features:

* A **budget**, which represents the maximum amount of time a CBS can
  dedicate to serve the aperiodic tasks given to it before triggering a
  deadline recalculation;
* A **period**, which is used to calculate the absolute deadline of
  the server; and
* A **job queue** dedicated to receive the aperiodic tasks (i.e. jobs)
  that it should execute.

.. note::
  The name *Constant Bandwidth* refers to the fact the `Utilization factor`
  of the server (i.e. execution time / period) remains the same at all times,
  thus enabling a more precise estimation of the taskset system usage. 

When a new job is pushed to the job queue, the CBS enters the active state,
removes the job from the queue and executes it. It then proceeds to the next
entry in the queue, if any, repeating the procedure over and over until the
queue becomes empty. When this finally happens the CBS becomes inactive,
remaining away from the CPU until a new job being pushed activates it again. 

The **budget** and **period** are used to manage the CBS absolute deadline
as the execution goes by. The period is used to calculate the value of a 
new deadline, while the budget being entirely consumed or meeting a certain
condition when a new job comes in is what triggers these calculations.

The budget is consumed when jobs get executed and is represented on the
Zephyr implementation as a timer. When the job finishes before the timer
expires, the budget is updated to a new value and the timer is stopped.
Conversely, if the timer expires before the job finishes the budget is
said to have run out, therefore being replenished to its full capacity
and triggering a deadline recalculation.

.. note::
  If even with the new (postponed) deadline the CBS remains with the
  highest priority among the taskset, it will resume its execution normally
  after the recalculation. The budget expiring event effectively serves to
  progressively lower the CBS priority, allowing other periodic tasks to
  take the CPU when it takes too long to execute the jobs assigned to it.

The fact that the CBS tries to keep execution after its budget runs out
is one of the things that differentiates it from other real-time servers.
Such behavior makes it a *work-conserving* algorithm.

CBS condition
=============

One of the key differences of CBS (compared to other real-time servers)
is that, every time a new job is pushed to the queue, the server checks
a certain condition to know whether or not it should replenish its own
budget and recalculate its deadline before serving the new job. Such
condition is that the server should be idle (i.e. with an empty queue)
prior to the new job arrival and that the following should be true:

``Cs >= (ds - rjob) * (Qs / T)``

Where:

* **Cs** is the current budget level.
* **ds** is the absolute deadline of the server.
* **rjob** is the job arrival instant. 
* **Qs** is the maximum budget the server can get.
* **T** is the server period.

If both parts are true, the budget Cs will be recharged to the maximum
value Qs and the deadline will be recalculated as rjob + T. If not, the
job will be served with the previous budget and deadline values.

How to Use the CBS
******************

The API consists of basically two elements, :c:macro:`K_CBS_DEFINE` to
statically define a CBS and :c:func:`k_cbs_push_job` function to insert
jobs on its job queue. Jobs are regular C functions executed within a
thread context that should **not** loop indefinitely (as it will prevent
further jobs on the queue to ever be reached), return nothing and accept 
one void pointer as the argument. A simple example of a job can be seen
below:

.. code-block:: c

  // some struct with a counter variable
  typedef struct {
      int counter;
  } job_t;

  // the job itself
  void job_function(void *arg){
    job_t *job = (job_t *) arg;
    printf("counter is %d.\n", job->counter);
    job->counter++;
  }

Assuming we create a server ``cbs_1`` as given below with a maximum
budget of, say, 50 milliseconds and a period of 200 milliseconds, we
can then push ``job_function`` to it by executing :c:func:`k_cbs_push_job`:

.. code-block:: c

  #define BUDGET        K_MSEC(50)
  #define PERIOD        K_MSEC(200)
  #define EDF_PRIORITY  5

  job_t arg = { .counter = 0 };

  K_CBS_DEFINE(cbs_1, BUDGET, PERIOD, EDF_PRIORITY);

  void periodic_thread_function(void *a1, void *a2, void *a3){
    while(true){
      k_cbs_push_job(&cbs_1, job_function, &arg, K_NO_WAIT);
      k_sleep(K_SECONDS(2));
    }
  }

The ``periodic_thread_function``, when executed by a thread, will then
push a job to the ``cbs_1`` queue every two seconds. The job queue is
implemented intenally as a regular message queue, and the last argument
passed to the push function (``K_NO_WAIT`` in this case) is meant to
indicate how much time should the function wait trying to push a job to
this queue.

.. note::
  one can call :c:func:`k_cbs_push_job` from an ISR or a thread context,
  but ISRs should **not** attempt to wait trying to push a job on the CBS
  queue. Moreover, jobs are executed within a system thread context.

It should be noted the last parameter of ``

Logging CBS events
==================

The CBS API provides a logging feature of the main events regarding the
server execution. Those can be viewed on the chosen console by enabling
:kconfig:option:`CONFIG_CBS_LOG` on the ``prj.conf`` file or through
``menuconfig``. The following events are supported and, although registered
synchronously as they happen, will be effectivelly logged on-screen by a
low-priority background task by default, in a strategy to keep the overhead
as minimal as possible. 

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

The example below shows the output for the target xiao_esp32c3. The
value alongside the event log is the budget level, in hardware cycles.
The CBS thread does an underlying conversion from timeout units passed
on :c:macro:`K_CBS_DEFINE` (e.g. :c:macro:`K_USEC`) to ensure units compatibility
with :c:func:`k_thread_deadline_set()`, which currently accepts only hardware
cycles.

.. code-block:: console

  [00:00:47.069,000] <inf> CBS: cbs_1     J_PUSH  25660       // first job is pushed to the queue
  [00:00:47.069,000] <inf> CBS: cbs_1     B_COND  160000      // condition is met, budget is replenished
  [00:00:47.069,000] <inf> CBS: cbs_1     J_PUSH  160000      // other job is pushed
  [00:00:47.069,000] <inf> CBS: cbs_1     SWT_TO  160000      // CBS thread enters CPU
  [00:00:47.073,000] <inf> CBS: cbs_1     J_COMP  104669      // first job completed
  [00:00:47.077,000] <inf> CBS: cbs_1     J_COMP  25687       // second job completed
  [00:00:47.077,000] <inf> CBS: cbs_1     SWT_AY  25687       // CBS thread leaves CPU





Configuration Options
**********************

Related configuration options:

* :kconfig:option:`CONFIG_CBS`
* :kconfig:option:`CONFIG_CBS_LOG`
* :kconfig:option:`CONFIG_CBS_THREAD_STACK_SIZE`
* :kconfig:option:`CONFIG_CBS_QUEUE_LENGTH`
* :kconfig:option:`CONFIG_CBS_INITIAL_DELAY`
* :kconfig:option:`CONFIG_CBS_CONDITION_SHIFT_AMOUNT`
* :kconfig:option:`CONFIG_CBS_THREAD_MAX_NAME_LEN`

API Reference
**************

.. doxygengroup:: cbs_apis
