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
    cbs_cycle_t current;
    cbs_cycle_t max;
} cbs_budget_t;


typedef struct {
    struct k_timer *timer;
    struct k_msgq *queue;
    struct k_thread *thread;
    cbs_budget_t budget;
	cbs_cycle_t period;
    cbs_cycle_t abs_deadline;
	cbs_cycle_t start_cycle;
    #ifdef CONFIG_CBS_LOG
    char name[CONFIG_CBS_THREAD_MAX_NAME_LEN];
    #endif
} cbs_t;


void k_cbs_push_job(cbs_t *cbs, cbs_callback_t job_function, void *job_arg, k_timeout_t timeout);

#define K_CBS_DEFINE(cbs_id, s_budget, s_period, s_static_priority)                 \
    K_MSGQ_DEFINE(queue_##cbs_id, sizeof(cbs_job_t), CONFIG_CBS_QUEUE_LENGTH, 1);   \
    static cbs_t cbs_id = {                                                         \
        .queue = &queue_##cbs_id,                                                   \
        .budget = { .current = s_budget, .max = s_budget },                         \
        .period = s_period                                                          \
    };                                                                              \
    static struct k_timer timer_##cbs_id;                                           \
    K_THREAD_DEFINE(thread_##cbs_id, CONFIG_CBS_THREAD_STACK_SIZE, cbs_thread,      \
        (void *) STRINGIFY(cbs_id), (void *) &cbs_id, (void *) &timer_##cbs_id,     \
        s_static_priority, 0, CONFIG_CBS_INITIAL_DELAY)


#endif /* CONFIG_CBS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS */