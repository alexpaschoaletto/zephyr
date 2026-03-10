/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/kernel/deadline_tracer_overhead.h>
#include <math.h>

typedef struct {
    char name[10];
    cbs_cycle_t samples[DEADLINE_TRACER_OVERHEAD_BUF_SIZE];
    cbs_cycle_t max;
    int count;
} overhead_t;

overhead_t cbs_budget_timer = {.name = "B_ROUT"};
overhead_t cbs_switched_to = {.name = "SWT_TO"};
overhead_t cbs_switched_away = {.name = "SWT_AY"};

cbs_cycle_t over_start;

void calculate_overhead(overhead_t *over){
    double sum = 0.0, average= 0.0;
    double delta = 0.0, deviation = 0.0;
    double std_dev = 0.0;
    double n = (double)((over->count)? over->count : DEADLINE_TRACER_OVERHEAD_BUF_SIZE);
	cbs_cycle_t local_max = 0;

    for(int i = 0; i < n; i++){
        sum += (double) over->samples[i];
        if(over->samples[i] > local_max){
            local_max = over->samples[i];
        }
    }

    average = sum / n;
    
    for(int i = 0; i < n; i++){
        delta = ((double) over->samples[i] - average);
        deviation += (delta * delta);
    }

    std_dev = sqrt(deviation / n);

    if(local_max > over->max){
		over->max = local_max;
        printk("%s  %.2f (dev: %.2f)   max: %llu (new highest)\n", over->name, average, std_dev, local_max);
    } else {
        printk("%s  %.2f (dev: %.2f)   max: %llu\n", over->name, average, std_dev, local_max);
    }
}

void trace_timer_overhead(cbs_cycle_t overhead){
    cbs_budget_timer.samples[cbs_budget_timer.count] = overhead;
    cbs_budget_timer.count = (cbs_budget_timer.count + 1) % DEADLINE_TRACER_OVERHEAD_BUF_SIZE;
    if(!cbs_budget_timer.count) calculate_overhead(&cbs_budget_timer);
}

void trace_switched_to_overhead(cbs_cycle_t overhead){
    cbs_switched_to.samples[cbs_switched_to.count] = overhead;
    cbs_switched_to.count = (cbs_switched_to.count + 1) % DEADLINE_TRACER_OVERHEAD_BUF_SIZE;
    if(!cbs_switched_to.count) calculate_overhead(&cbs_switched_to);
}

void trace_switched_away_overhead(cbs_cycle_t overhead){
    cbs_switched_away.samples[cbs_switched_to.count] = overhead;
    cbs_switched_away.count = (cbs_switched_to.count + 1) % DEADLINE_TRACER_OVERHEAD_BUF_SIZE;
    if(!cbs_switched_away.count) calculate_overhead(&cbs_switched_away);
}