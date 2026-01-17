/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_KERNEL_DEADLINE_H_
#define ZEPHYR_INCLUDE_KERNEL_DEADLINE_H_

#include <zephyr/kernel.h>

#define HAS_PERIOD(thread)                              (thread->base.period > 0)
#define HAS_CBS_ACTIVE(thread)                          (thread->base.cbs.is_active)
#define HAS_CBS_ENABLED(thread)                         (thread->base.cbs.max_budget > 0)
#define HAS_DEADLINE_CALLBACK(thread)                   (thread->base.deadline_miss_callback != NULL)
#define IS_FIRST_CYCLE(thread)                          (thread->base.activation_tick == 0)
#define DEADLINE_IS_ZERO(deadline)                      (K_TIMEOUT_EQ(deadline, K_NO_WAIT))

struct k_cbs {
    struct k_thread *thread;
    struct k_timer budget_timer;
    int64_t relative_deadline;
    int64_t current_budget;
    int64_t max_budget;
    int64_t start_tick;
    bool is_active;
};

/**
 * @brief called when an EDF task enters the CPU.
 * 
 * This function simply re-starts the budget timer
 * when a task, that is currently executing with
 * both EDF and CBS, enters the CPU.
 * 
 * The timer is restarted with the current budget
 * as the expiry period.
 */
void z_cbs_switched_in(struct k_cbs *cbs);

/**
 * @brief called when an EDF task leaves the CPU.
 * 
 * This function simply stops the budget timer
 * when a task that is currently executing with
 * both EDF and CBS configured leaves the CPU.
 * 
 * The current budget is subtracted from the elapsed
 * time since the start tick.
 */
void z_cbs_switched_out(struct k_cbs *cbs);

#endif /* ZEPHYR_INCLUDE_KERNEL_DEADLINE_H_ */