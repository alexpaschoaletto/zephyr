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

edf_t task1 = {
	.id = '1',
	.counter = 0,
	.initial_delay_msec = 0,
	.rel_deadline_msec = 7000,
	.period_msec = 7000,
	.wcet_msec = 4000
};

job_t job1 = {
	.id = 'C',
	.counter = 0,
	.wcet_msec = 4000
};


void trigger(struct k_timer *timer){
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


void cycle(char id, uint32_t wcet){
	printf("[%c-", id);
	k_busy_wait(wcet/10);
	for(int i = 0; i < 9; i++){
		printf("%c-", id);
		k_busy_wait(wcet/10);
	}
	printf("%c]-", id);
}


void job_function(void *job_params){
	job_t *job = (job_t *) job_params;
	trace(job->id, job->counter, START);
	uint32_t wcet = MSEC_TO_USEC(job->wcet_msec);
	cycle(job->id, wcet);
	trace(job->id, job->counter, END);
	job->counter++;
}


void thread_function(void *task_props, void *a2, void *a3){
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

K_TIMER_DEFINE(
	trace_timer,
	print_trace,
	NULL
);

K_THREAD_DEFINE(
	task1_thread, 2048,
	thread_function,
	&task1, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


K_CBS_DEFINE(
	cbs_1,
	K_MSEC(3500),
	K_MSEC(8000),
	EDF_PRIORITY
);



int main(void){
	k_sleep(K_SECONDS(1));
	report_cbs_settings();

	k_thread_start(task1_thread);
	k_timer_start(&trace_timer, K_SECONDS(20), K_SECONDS(20));

	k_sleep(K_MSEC(3000));
	for(;;){
		k_cbs_push_job(&cbs_1, job_function, &job1, K_FOREVER);
		trace(job1.id, job1.counter, TRIGGER);
		k_sleep(K_MSEC(10000));
	}

	return 0;
}
