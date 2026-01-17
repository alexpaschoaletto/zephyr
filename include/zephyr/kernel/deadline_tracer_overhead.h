/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_KERNEL_DEADLINE_OVERHEAD_H_
#define ZEPHYR_INCLUDE_KERNEL_DEADLINE_OVERHEAD_H_

#include <zephyr/kernel.h>

#define DEADLINE_TRACER_OVERHEAD_BUF_SIZE        100

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
typedef uint64_t cbs_cycle_t;
#else
typedef uint32_t cbs_cycle_t;
#endif

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
#define cbs_get_now() k_cycle_get_64()
#else
#define cbs_get_now() k_cycle_get_32()
#endif

extern cbs_cycle_t over_start;

#define trace_overhead_start()  (over_start = cbs_get_now())

void trace_timer_overhead(cbs_cycle_t overhead);
void trace_switched_to_overhead(cbs_cycle_t overhead);
void trace_switched_away_overhead(cbs_cycle_t overhead);

#endif /* ZEPHYR_INCLUDE_KERNEL_DEADLINE_OVERHEAD_H_ */