/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_KERNEL_DEADLINE_TRACER_H_
#define ZEPHYR_INCLUDE_KERNEL_DEADLINE_TRACER_H_

#include <zephyr/kernel.h>
#include <string.h>

#define DEADLINE_TRACER_BUF_SIZE 100

#define TRIGGER  0
#define START    1
#define END      2
#define B_ROUT   3
#define B_COND   4
#define B_CHECK  5
#define SWT_TO	 6
#define SWT_AY	 7

void begin_trace(void);
void reset_trace(void);
void print_trace(void);
void trace(char thread_id, int thread_counter, int event);
void trace_cbs(char thread_id, int64_t current_budget, int64_t deadline, int64_t cbs_budget, int64_t cbs_period, int event);

#endif /* ZEPHYR_INCLUDE_KERNEL_DEADLINE_TRACER_H_ */