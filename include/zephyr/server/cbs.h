/* 
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

/**
 * @file
 *
 * @brief Public Constant Bandwidth Server (CBS) APIs.
 */

#ifndef ZEPHYR_CBS
#define ZEPHYR_CBS

#include <zephyr/kernel.h>
#include <zephyr/server/internal/cbs_internal.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CBS

/**
 * @defgroup cbs_apis Constant Bandwidth Server (CBS) APIs
 * @{
 */

typedef void (* cbs_callback_t)(void *arg);

/**
 * @cond INTERNAL_HIDDEN
 */

typedef struct {
    cbs_callback_t function;
    void *arg;
} cbs_job_t;


typedef struct {
    cbs_cycle_t current;
    cbs_cycle_t max;
} cbs_budget_t;


typedef struct {
    k_timeout_t budget;
    k_timeout_t period;
} cbs_arg_t;

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

/** @endcond */

/** 
 * @brief pushes a job to a CBS queue.
 *
 * This routine pushes an aperiod task (i.e. function) to the CBS job queue, which will
 * be later served in a FIFO manner. The job queue can take up to
 * @kconfig{CONFIG_CBS_QUEUE_LENGTH} jobs at once. 
 *
 * @param cbs Name of the CBS job queue.
 * @param job_function Function of the job.
 * @param cbs_period Argument to be passed to the job function.
 * @param timeout Waiting period to push the job, or one of the special values K_NO_WAIT and K_FOREVER. 
 *
 * @retval 0 Job pushed to the CBS queue.
 * @retval -ENOMSG if CBS is not defined, returned without waiting or CBS queue purged.
 * @retval -EAGAIN if waiting period timed out. 
 */
int k_cbs_push_job(cbs_t *cbs, cbs_callback_t job_function, void *job_arg, k_timeout_t timeout);

/**
 * @brief Statically define and initialize a Constant Bandwidth Server (CBS).
 *
 * The CBS is a Real-Time server meant to work alongside a dynamic scheduling
 * algorithm - in Zephyr's case, that's EDF - enabling aperiodic tasks to be
 * served alongside the periodic ones.
 * 
 * These aperiodic tasks, also known as jobs, are usually event-driven and
 * therefore have unkown arrival and execution times, which make them hard
 * to be directly scheduled under the EDF policy alone. The CBS then acts as
 * a wrapper, having its own deadline and period for schedulability analysis
 * and a queue meant for receiving the jobs.
 *
 * When a new job is pushed to its own queue, the CBS performs a few condition
 * checks and enters the kernel run-queue, awaiting its execution just like any 
 * other periodic task. The kernel will then execute the CBS thread, and therefore
 * the new job, once it features the earliest absolute deadline among the ready
 * threads.
 *
 * If there are no jobs left to execute, the CBS switches to the pending state,
 * remaining inactive until a new job is pushed to its queue. Once created, the
 * CBS is accessible through its job queue:
 *
 * @code extern const struct k_msgq <name>; @endcode
 *
 * @param cbs_name Name of the CBS job queue.
 * @param cbs_budget Budget of the CBS thread, in system ticks.
 * Used for triggering deadline recalculations.
 * @param cbs_period Period of the CBS thread. in system ticks.
 * Used for recalculating the absolute deadline.
 * @param cbs_static_priority Static priority of the CBS thread. 
 *
 * @note The CBS is meant to be used alongside the EDF policy, which in Zephyr
 * is effectively used as a "tie-breaker" when two threads of equal static priorities
 * are ready for execution. Therefore it is recommended that all user threads feature
 * the same preemptive static priority (e.g. 5) in order to ensure the scheduling
 * to work as expected, and that this same value is passed as @a cbs_static_priority.
 */
#define K_CBS_DEFINE(cbs_name, cbs_budget, cbs_period, cbs_static_priority)           \
    K_MSGQ_DEFINE(queue_##cbs_name, sizeof(cbs_job_t), CONFIG_CBS_QUEUE_LENGTH, 1);   \
    static struct k_timer timer_##cbs_name;                                           \
    static cbs_t cbs_name = {                                                         \
        .queue = &queue_##cbs_name,                                                   \
        .timer = &timer_##cbs_name                                                    \
    };                                                                                \
    static cbs_arg_t args_##cbs_name = {                                              \
        .budget = cbs_budget,                                                         \
        .period = cbs_period                                                          \
    };                                                                                \
    K_THREAD_DEFINE(thread_##cbs_name, CONFIG_CBS_THREAD_STACK_SIZE, cbs_thread,      \
        (void *) STRINGIFY(cbs_name), (void *) &cbs_name, (void *) &args_##cbs_name,  \
        cbs_static_priority, 0, CONFIG_CBS_INITIAL_DELAY)

/** @} */ /* end of Constant Bandwidth Server (CBS) APIs */

#endif /* CONFIG_CBS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS */