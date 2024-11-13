/*
 * Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/server/cbs.h>
#include <string.h>
#include "lib/helper.h"
#include "lib/tracer.h"

/*
 * set EXAMPLE to 1 for a simple example of 1 EDF thread and 1 CBS.
 * set EXAMPLE to 2 for a more complex example of 2 EDF threads and 1 CBS.
*/
#define EXAMPLE	1


void trigger(struct k_timer *timer)
{
	/*
		This function triggers a cycle for
		the periodic EDF thread.

		the message itself is not relevant -
		the *act* of sending the message is.
		That's what unblocks the thread to
		execute a cycle.
	*/
	const char t = 1;
	edf_t *task = (edf_t *) k_timer_user_data_get(timer);
	int deadline = MSEC_TO_CYC(task->rel_deadline_msec);
	k_thread_deadline_set(task->thread, deadline);
	k_msgq_put(&task->queue, &t, K_NO_WAIT);
	trace(task->id, task->counter, TRIGGER);
}


void cycle(char id, uint32_t wcet)
{
	printf("[%c-", id);
	k_busy_wait(wcet/10);
	for(int i = 0; i < 9; i++){
		printf("%c-", id);
		k_busy_wait(wcet/10);
	}
	printf("%c]-", id);
}


void job_function(void *job_params)
{
	job_t *job = (job_t *) job_params;
	trace(job->id, job->counter, START);
	uint32_t wcet = MSEC_TO_USEC(job->wcet_msec);
	cycle(job->id, wcet);
	trace(job->id, job->counter, END);
}


void thread_function(void *task_props, void *a2, void *a3)
{
	char buffer[10];
	char message;

	edf_t *task = (edf_t *) task_props;
	task->thread = k_current_get();

	/* value passed to k_busy_wait needs to be in usec */
	uint32_t wcet = MSEC_TO_USEC(task->wcet_msec);

	k_msgq_init(&task->queue, buffer, sizeof(char), 10);
	k_timer_init(&task->timer, trigger, NULL);
	k_timer_user_data_set(&task->timer, (void *) task);
	k_timer_start(&task->timer, K_MSEC(task->initial_delay_msec), K_MSEC(task->period_msec));

	for(;;){
		k_msgq_get(&task->queue, &message, K_FOREVER);
		trace(task->id, task->counter, START);
		cycle(task->id, wcet);
		trace(task->id, task->counter, END);
		task->counter++;
	}
}


/****************************************************************************************************/
#if EXAMPLE == 1 									/* this is the taskset if you choose example 1. */

#define CBS_BUDGET 	K_MSEC(3000)
#define CBS_PERIOD 	K_MSEC(8000)

edf_t tasks[] = {
	{
		.id = '1',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 7000,
		.period_msec = 7000,
		.wcet_msec = 4000
	}
};

K_THREAD_DEFINE(task1, 2048, thread_function, &tasks[0], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);

/****************************************************************************************************/
#elif EXAMPLE == 2 									/* this is the taskset if you choose example 2. */

#define CBS_BUDGET 	K_MSEC(2000)
#define CBS_PERIOD 	K_MSEC(6000)

edf_t tasks[] = {
	{
		.id = '1',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 6000,
		.period_msec = 6000,
		.wcet_msec = 2000
	},{
		.id = '2',
		.counter = 0,
		.initial_delay_msec = 0,
		.rel_deadline_msec = 9000,
		.period_msec = 9000,
		.wcet_msec = 3000
	}
};

K_THREAD_DEFINE(task1, 2048, thread_function, &tasks[0], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);
K_THREAD_DEFINE(task2, 2048, thread_function, &tasks[1], NULL, NULL, EDF_PRIORITY, 0, INACTIVE);

#endif
/****************************************************************************************************/

K_CBS_DEFINE(cbs_1, CBS_BUDGET, CBS_PERIOD, EDF_PRIORITY);
K_TIMER_DEFINE(trace_timer, print_trace, NULL);


int main(void){
	k_sleep(K_SECONDS(1));
	k_timer_start(&trace_timer, K_SECONDS(20), K_SECONDS(20));

	report_cbs_settings();

	#if EXAMPLE == 1
	k_thread_start(task1);
	/*
	 * The CBS is most useful when a given code
	 * needs to execute at arbitrary intervals
	 * alongside a hard and periodic taskset.
	 * To demonstrate that, in example 1, we:
	 *
	 * - wait 3 seconds;
	 * - push a 4-second job;
	 * - wait 10 seconds;
	 * - push a 3-second job.
	 *
	 * So neither the execution time, nor the
	 * arrival instant, nor the amount of jobs
	 * are fixed to the eyes of the scheduler.
	 * Thus, if we were to deal with this job
	 * directly with EDF, it would be hard
	 * to ensure a predictable execution.
	 */
	job_t *job1 = create_job('C', 0, 4000);
	k_sleep(K_MSEC(3000));
	k_cbs_push_job(&cbs_1, job_function, job1, K_FOREVER);
	trace(job1->id, job1->counter, TRIGGER);

	job_t *job2 = create_job('C', 1, 3000);
	k_sleep(K_MSEC(10000));
	k_cbs_push_job(&cbs_1, job_function, job2, K_FOREVER);
	trace(job2->id, job2->counter, TRIGGER);

	destroy_job(job1);
	destroy_job(job2);

	#elif EXAMPLE == 2
	k_thread_start(task1);
	k_thread_start(task2);
	/*
	 * In example 2 we push the same job, but
	 * at a slightly different timing:
	 *
	 * - wait 2 seconds;
	 * - push a 3-second job;
	 * - wait 10 seconds;
	 * - push a 3-second job;
	 * - wait 8 seconds;
	 * - push a 1-second job.
	 *
	 * The values for both examples were
	 * tailored to showcase the CBS behavior
	 * under different circumstances.
	 */
	job_t *job1 = create_job('C', 0, 3000);
	k_sleep(K_MSEC(2000));
	k_cbs_push_job(&cbs_1, job_function, job1, K_FOREVER);
	trace(job1->id, job1->counter, TRIGGER);

	job_t *job2 = create_job('C', 1, 3000);
	k_sleep(K_MSEC(10000));
	k_cbs_push_job(&cbs_1, job_function, job2, K_FOREVER);
	trace(job2->id, job2->counter, TRIGGER);

	job_t *job3 = create_job('C', 2, 1000);
	k_sleep(K_MSEC(8000));
	k_cbs_push_job(&cbs_1, job_function, job3, K_FOREVER);
	trace(job3->id, job3->counter, TRIGGER);
	
	destroy_job(job1);
	destroy_job(job2);
	destroy_job(job3);
	#endif

	return 0;
}
