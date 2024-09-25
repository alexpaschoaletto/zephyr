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

/*
	Remember that for "pure" EDF results, all user
	threads must have the same static priority.
	That happens because EDF will be a tie-breaker
	among two or more ready tasks of the same static
	priority. An arbitrary positive number is chosen here.
*/
#define EDF_PRIORITY 5

typedef struct {
    char msg[100];
    uint32_t counter;
} job_t;

job_t job1 = { .msg = "[job]\t\tj1 on", .counter = 0 };
job_t job2 = { .msg = "[job]\t\tj2 on", .counter = 0 };

/*
	The CBS job must return void and receive a void*
	argument as parameter. The actual argument type is
	up to you, to be unveiled and used within the
	function.
*/

void job_function(void *arg){
    job_t *job = (job_t *) arg;
    printf("%s %s, %d\n\n", job->msg, CONFIG_BOARD_TARGET, job->counter);
    job->counter++;
}

/*
	A few suggestions to understand the CBS:

	1. Try changing the values of budget and period
	to see the effect on budget consumption,
	replenishing and condition checking. You'll see
	that the CBS is work-conserving, meaning that
	it should execute all its jobs one after the other
	as long as it holds the highest EDF priority of
	all periodic tasks.
	
	2. Try enabling the seconds CBS server to see
	which one executes its jobs first. You'll note that
	the amount of jobs pushed to each CBS should not
	be relevant, but rather the period each one is given.
	Thus, since cbs_2 has 600us of period vs 20ms of
	cbs_1, it will have priority over cbs_1 despite only
	receiving 1 job.

	If two servers have jobs pushed at similar instants,
	the one with smaller period will likely be the one
	with earliest absolute deadline and therefore be
	the one selected for first execution.

*/

K_CBS_DEFINE(cbs_1, K_MSEC(80), K_SECONDS(2), EDF_PRIORITY);
// K_CBS_DEFINE(cbs_2, K_USEC(100), K_USEC(600), EDF_PRIORITY);

int main(void){
	k_sleep(K_SECONDS(2));
	report_cbs_settings();
	
	for(;;){
		printf("\n");
        k_cbs_push_job(&cbs_1, job_function, &job1, K_NO_WAIT);
        k_cbs_push_job(&cbs_1, job_function, &job1, K_NO_WAIT);
        k_cbs_push_job(&cbs_1, job_function, &job1, K_NO_WAIT);
		// k_cbs_push_job(&cbs_2, job_function, &job2, K_NO_WAIT);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}