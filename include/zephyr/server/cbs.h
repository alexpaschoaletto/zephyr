/* 
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#ifndef ZEPHYR_CBS
#define ZEPHYR_CBS

#include <zephyr/kernel.h>
#include <zephyr/server/internal/cbs_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CBS

typedef void (* cbs_callback_t)(void *arg);


typedef struct {
    cbs_callback_t function;
    void *arg;
} cbs_job_t;


typedef struct {
    uint32_t current;
    uint32_t max;
} cbs_budget_t;


typedef struct {
    struct k_timer timer;
    struct k_msgq *queue;
    struct k_thread *thread;
	cbs_budget_t budget;
	sys_snode_t node;
	char name[20];
	uint32_t period;
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	uint64_t abs_deadline;
	uint64_t start_cycle;
    #else
    uint32_t abs_deadline;
    uint32_t start_cycle;
    #endif
} cbs_t;


void k_cbs_push_job(cbs_t *cbs, cbs_callback_t job_function, void *job_arg, k_timeout_t timeout);

#define K_CBS_DEFINE(cbs_id, s_name, s_budget, s_period, s_static_priority)         \
    static cbs_t cbs_id = {                                                         \
        .name = s_name,                                                             \
        .budget = { .current = s_budget, .max = s_budget },                         \
        .period = s_period                                                          \
    };                                                                              \
    K_MSGQ_DEFINE(queue_##cbs_id, sizeof(cbs_job_t), CONFIG_CBS_QUEUE_LENGTH, 1);   \
    K_THREAD_DEFINE(thread_##cbs_id, CONFIG_CBS_THREAD_STACK_SIZE, cbs_thread,      \
        (void *) &queue_##cbs_id, (void *) &cbs_id, NULL,                           \
        s_static_priority, 0, CONFIG_CBS_INITIAL_DELAY)


#endif /* CONFIG_CBS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS */