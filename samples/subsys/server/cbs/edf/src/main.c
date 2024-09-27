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

edf_t task1 = {
	.id = 1,
	.rel_deadline_msec = 30,
	.period_msec = 2000,
	.wcet_msec = 5
};

edf_t task2 = {
	.id = 2,
	.rel_deadline_msec = 14,
	.period_msec = 6000,
	.wcet_msec = 1
};


void grow(edf_t *task, uint32_t amount){
	// task->wcet_msec += amount;
	task->rel_deadline_msec += amount;
}


void trigger(edf_t *task){
	const char t = 1;
	int deadline = MSEC_TO_CYC(task->rel_deadline_msec);
	k_msgq_put(&task->queue, &t, K_NO_WAIT);
	k_thread_deadline_set(task->thread, deadline);
}


void trigger_one(struct k_timer *timer){
	edf_t *task = (edf_t *) k_timer_user_data_get(timer);
	trigger(task);
}


void trigger_all(struct k_timer *timer){
	printk(" \n");
	trigger(&task1);
	trigger(&task2);
	grow(&task2, 5000);
}


void thread_function(void *task_props, void *a2, void *a3){
	char buffer[10];
	char message;
	int counter = 0;
	uint32_t deadline = 0;
	uint64_t start = 0;
	uint64_t end = 0;

	edf_t *task = (edf_t *) task_props;
	task->thread = k_current_get();

	k_thread_custom_data_set((void *) task);
	k_msgq_init(&task->queue, buffer, sizeof(char), 10);
	// k_timer_init(&task->timer, trigger_one, NULL);
	// k_timer_user_data_set(&task->timer, (void *) task);
	// k_timer_start(&task->timer, K_NO_WAIT, K_MSEC(task->period_msec));
	printf("[ %d ] ready.\n", task->id);

	for(;;){
		/*
			The periodic thread has no period, ironically.
			What makes it periodic is that it has a timer
			associated with it that regularly sends new
			messages to the thread queue (e.g. every two
			seconds).
		*/
		k_msgq_get(&task->queue, &message, K_FOREVER);
		start = k_cycle_get_64();
		counter++;
		deadline = task->thread->base.prio_deadline;
		k_busy_wait(MSEC_TO_USEC(task->wcet_msec));
		end = k_cycle_get_64();
		printk("[ %d ] %d\tstart %llu\t end %llu\t dead %d\n", task->id, counter, start, end, deadline);
	}
}


K_THREAD_DEFINE(
	task1_thread, 2048,
	thread_function,
	&task1, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


K_THREAD_DEFINE(
	task2_thread, 2048,
	thread_function,
	&task2, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


K_TIMER_DEFINE(master_timer, trigger_all, NULL);

int main(void){
	k_sleep(K_SECONDS(1));
	report_cbs_settings();
	k_thread_start(task1_thread);
	k_thread_start(task2_thread);

	k_timer_start(&master_timer, K_SECONDS(3), K_SECONDS(3));
	return 0;
}


// void sys_trace_thread_switched_in_user(){                                     //should log every time the test thread enters the CPU
// 	edf_t *task = (edf_t *) k_thread_custom_data_get();
// 	if(!task) return;
// 	int64_t cycle = k_cycle_get_64();
// 	printk("[ %d ]\t in  %llu\n", task->id, cycle);
// }


// void sys_trace_thread_switched_out_user(){                                    //should log every time the test thread leaves the CPU
//     edf_t *task = (edf_t *) k_thread_custom_data_get();
// 	if(!task) return;
// 	int64_t cycle = k_cycle_get_64();
// 	printk("[ %d ]\t out %llu\n", task->id, cycle);
// }
