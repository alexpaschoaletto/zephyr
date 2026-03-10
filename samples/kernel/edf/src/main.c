/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/kernel/deadline_tracer.h>
#include "lib/helper.h"

/*
 * set EXAMPLE to 1 for a simple example of 1 EDF thread and 1 CBS.
 * set EXAMPLE to 2 for a more complex example of 2 EDF threads and 1 CBS.
 */
#define EXAMPLE 1

/*
 * the tasks and CBS will have their execution times, period and
 * relative deadlines all multiplied by U, in milliseconds.
 */
#define U 1000

bool finished_cbs_jobs = false;

void cbs_function(void *cbs_props, void *a2, void *a3)
{
	edf_t *cbs = (edf_t *) cbs_props;
	k_tid_t thread = k_current_get();

	k_thread_cbs_budget_set(thread, K_MSEC(cbs->budget_msec));
	k_thread_period_set(thread, K_MSEC(cbs->period_msec));

	if(cbs->initial_delay_msec > 0){
		k_sleep(K_MSEC(cbs->initial_delay_msec));
	}

	for(;HAS_JOBS_LEFT(cbs); cbs++){
		k_thread_deadline_set(thread, K_MSEC(cbs->rel_deadline_msec));
		trace(cbs->id, cbs->counter, TRIGGER);
		k_reschedule();

		trace(cbs->id, cbs->counter, START);
		cycle(cbs->id, MSEC_TO_USEC(cbs->wcet_msec));
		// for(;;) cycle(cbs->id, MSEC_TO_USEC(cbs->wcet_msec));	/* OVERHEAD MEASUREMENT (use this line instead to consume budget indefinitely) */
		trace(cbs->id, cbs->counter, END);

		k_thread_period_set(thread, K_MSEC(cbs->period_msec));
	}

	finished_cbs_jobs = true;
}


void thread_function(void *task_props, void *a2, void *a3)
{
	edf_t *task = (edf_t *)task_props;

	task->thread = k_current_get();
	task->counter = 0;

	uint32_t wcet = MSEC_TO_USEC(task->wcet_msec);

	k_thread_period_set(task->thread, K_MSEC(task->period_msec));
	k_sleep(K_USEC(500)); /* let other threads set up */

	for(;!finished_cbs_jobs;){
		k_thread_deadline_set(task->thread, K_MSEC(task->rel_deadline_msec));
		k_reschedule();

		trace(task->id, task->counter, START);
		cycle(task->id, wcet);
		trace(task->id, task->counter, END);

		task->counter++;
	}
}


/***********************************************************************************************/
#if EXAMPLE == 1 /* this is the taskset if you choose example 1. */

#define CBS_BUDGET	(3 * U)
#define CBS_PERIOD	(8 * U)

edf_t cbs[] = {
	{
		.id = 'A',
		.initial_delay_msec = 3 * U,
		.rel_deadline_msec = CBS_PERIOD,
		.budget_msec = CBS_BUDGET,
		.period_msec = 10 * U,			// here this will be used as the time between job activations
		.wcet_msec = 4 * U
	},{
		.id = 'B',
		.rel_deadline_msec = CBS_PERIOD,	
		.budget_msec = CBS_BUDGET,
		.period_msec = 0,				// here this will be used as the time between job activations
		.wcet_msec = 3 * U
	},
	END_OF_JOBS
};

edf_t tasks[] = {
	{
		.id = '1',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 7 * U,
		.period_msec = 7 * U,
		.wcet_msec = 4 * U
	}
};

K_THREAD_DEFINE(cbs1, 2048, cbs_function, (void *) cbs, NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task1, 2048, thread_function, &tasks[0], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);

/***********************************************************************************************/
#elif EXAMPLE == 2 /* this is the taskset if you choose example 2. */

#define CBS_BUDGET (2 * U)
#define CBS_PERIOD (6 * U)

edf_t cbs[] = {
	{
		.id = 'A',
		.initial_delay_msec = 2 * U,
		.rel_deadline_msec = CBS_PERIOD,
		.budget_msec = CBS_BUDGET,
		.period_msec = 10 * U,			// here this will be used as the time between job activations
		.wcet_msec = 3 * U
	},{
		.id = 'B',
		.rel_deadline_msec = CBS_PERIOD,	
		.budget_msec = CBS_BUDGET,
		.period_msec = 8 * U,			// here this will be used as the time between job activations
		.wcet_msec = 3 * U
	},{
		.id = 'C',
		.rel_deadline_msec = CBS_PERIOD,	
		.budget_msec = CBS_BUDGET,
		.period_msec = 0,				// here this will be used as the time between job activations
		.wcet_msec = 1 * U
	},
	END_OF_JOBS
};

edf_t tasks[] = {
	{
		.id = '1',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 6 * U,
		.period_msec = 6 * U,
		.wcet_msec = 2 * U
	},{
		.id = '2',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 9 * U,
		.period_msec = 9 * U,
		.wcet_msec = 3 * U
	}
};

K_THREAD_DEFINE(cbs1, 2048, cbs_function, (void *) cbs, NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task1, 2048, thread_function, &tasks[0], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task2, 2048, thread_function, &tasks[1], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);

#endif

int main(void)
{
	wait_before_begin(5000);
	begin_trace();

#if EXAMPLE == 1
	k_thread_start(cbs1);
	k_thread_start(task1);

#elif EXAMPLE == 2
	k_thread_start(cbs1);
	k_thread_start(task1);
	k_thread_start(task2);

#endif
	while(!finished_cbs_jobs) k_sleep(K_MSEC(30 * U));
	print_trace();
	return 0;
}
