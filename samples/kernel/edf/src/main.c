/*
 * Copyright (c) 2025 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define BUF_SIZE 10
#define INACTIVE -1
#define PRIORITY 5

int deadline_misses = 0;

void oh_no(void *misses)
{
	int *miss = (int *) misses;
	*miss += 1;
	printk("[%lld]\t Oh no! Deadline missed for the %dth time.\n", k_uptime_get(), *miss);
}


void thread_function(void *name, void *C, void *P)
{
	char *thread_name = (char *) name;
	int execution_time = (int) C * USEC_PER_MSEC;
	int period = (int) P;

	struct k_thread *this_thread = k_current_get();
	int cycle = 0;

	k_thread_period_set(this_thread, K_MSEC(period));
	k_thread_deadline_miss_callback_set(this_thread, oh_no, (void *) &deadline_misses);
	printk("[%s]\t execution time: %dms, period: %dms\n", thread_name, execution_time / USEC_PER_MSEC, period);

	for(;;){
		k_thread_deadline_set(this_thread, K_MSEC(period));
		k_reschedule();
		printk("[%s]\t start %d\t (at %lldms)\n", thread_name, cycle, k_uptime_get());
		k_busy_wait(execution_time);
		printk("[%s]\t end   %d\t (at %lldms)\n", thread_name, cycle, k_uptime_get());
		cycle++;
	}
}


K_THREAD_DEFINE(threadA, 2048, thread_function,
	"A", 1000, 5000, PRIORITY, 0, 0);
