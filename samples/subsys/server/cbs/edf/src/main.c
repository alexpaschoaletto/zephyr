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
	.period_cyc = SEC_TO_CYC(2),
	.rel_deadline_cyc = USEC_TO_CYC(600)
};

job_t job1 = {
	.msg = "[job]\t\tj1 on",
	.counter = 0
};


void job_function(void *arg){
    job_t *job = (job_t *) arg;
    printf("%s %s, %d\n\n", job->msg, CONFIG_BOARD_TARGET, job->counter);
    job->counter++;
}

K_CBS_DEFINE(
	cbs_1, K_MSEC(10), K_MSEC(20),
	EDF_PRIORITY
);

void thread_function(void *edf_props, void *a2, void *a3){
	printf("starting task\n");
	k_tid_t this_thread = k_current_get();
	edf_t *edf = (edf_t *) edf_props;
	
	for(;;){
        k_cbs_push_job(&cbs_1, job_function, &job1, K_NO_WAIT);
        k_sleep(K_CYC(edf->period_cyc));
		k_thread_deadline_set(this_thread, edf->rel_deadline_cyc);
	}
}

K_THREAD_DEFINE(
	task1_thread, 2048, thread_function,
	&task1, NULL, NULL,
	EDF_PRIORITY, 0, INACTIVE
);


int main(void){
	k_sleep(K_SECONDS(2));
	
	report_cbs_settings();
	k_thread_deadline_set(task1_thread, task1.rel_deadline_cyc);
	k_cbs_push_job(&cbs_1, job_function, &job1, K_NO_WAIT);

	int32_t now = k_cycle_get_32();
	printf(
		"now: \t%u\ncbs_1: \t%u\ntask:\t%u\n", now,
		cbs_1.thread->base.prio_deadline,
		task1_thread->base.prio_deadline
	);
	
	k_thread_start(task1_thread);
	return 0;
}