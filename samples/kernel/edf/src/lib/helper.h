/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

/*
 * Remember that for "pure" EDF results, all
 * user threads must have the same static priority.
 * That happens because EDF will be a tie-breaker
 * among two or more ready tasks of the same static
 * priority. An arbitrary positive number is chosen here.
 */
#define EDF_PRIORITY            (5)
#define INACTIVE                (-1)
#define MSEC_TO_CYC(msec)       (k_ms_to_cyc_near32(msec))
#define MSEC_TO_USEC(msec)      (msec * USEC_PER_MSEC)
#define HAS_JOBS_LEFT(cbs)      (cbs->id != '\0')
#define END_OF_JOBS             {.id = '\0'}

typedef struct {
	char id;
	int counter;
	int32_t initial_delay_msec;
	int32_t rel_deadline_msec;
    int32_t budget_msec;
	int32_t period_msec;
	uint32_t wcet_msec;
	k_tid_t thread;
	struct k_msgq queue;
	struct k_timer timer;
} edf_t;

void wait_before_begin(int milliseconds){
	for(int i = 0; i < milliseconds; i += 1000){
		k_sleep(K_MSEC(1000));
		printk(".");
	}
	printk("\n");
}

void cycle(char id, uint32_t wcet)
{
	printf("[%c-", id);
	k_busy_wait(wcet / 10);
	for (int i = 0; i < 8; i++) {
		printf("%c-", id);
		k_busy_wait(wcet / 10);
	}
	printf("%c]-", id);
	k_busy_wait(wcet / 10);
}

#endif /* _APP_HELPER_H_ */