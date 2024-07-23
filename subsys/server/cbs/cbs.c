/*
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/server/internal/cbs_internal.h>
#include <zephyr/server/cbs.h>
#include <string.h>

#ifdef CONFIG_CBS_LOG
#include <cbs_log.h>
#endif


static void cbs_budget_update_consumption(cbs_t *cbs){
    if(!cbs || !cbs->start_cycle) return;
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    uint64_t now = k_cycle_get_64();
    #else
    uint32_t now = k_cycle_get_32();
    #endif
    uint32_t budget_used = (uint32_t)(now - cbs->start_cycle);
    if(cbs->budget.current > budget_used){
        cbs->budget.current -= budget_used;
    } else {
        cbs->budget.current = 0;
    }
}


static void cbs_budget_ran_out(struct k_timer *timer){
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    uint64_t now = k_cycle_get_64();
    #else
    uint32_t now = k_cycle_get_32();
    #endif
    
    cbs_t *cbs = (cbs_t *) k_timer_user_data_get(timer);
    cbs->start_cycle = now;
    cbs->abs_deadline += cbs->period;
    cbs->budget.current = cbs->budget.max;
    k_thread_deadline_set(cbs->thread, (int) (cbs->abs_deadline - now));
    k_timer_start(timer, K_CYC(cbs->budget.current), K_NO_WAIT);
    
    #ifdef CONFIG_CBS_LOG
    cbs_trace(CBS_BUDGET_RAN_OUT, cbs);
    #endif
}


static void cbs_budget_stop(struct k_timer *timer){
    cbs_t *cbs = (cbs_t *) k_timer_user_data_get(timer);
    cbs_budget_update_consumption(cbs);
    cbs->start_cycle = 0;
}


static bool cbs_budget_restore_on_condition(cbs_t *cbs){
    if(!cbs_is_idle(cbs->thread)) return false;
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    uint64_t arrival = k_cycle_get_64();
    uint64_t deadline = cbs->abs_deadline;
    #else
    uint32_t arrival = k_cycle_get_32();
    uint32_t deadline = cbs->abs_deadline;
    #endif
    /*
        if arrival >= deadline, condition is instantly met. Otherwise,
        we need to check more stuff. Note we multiply both budget and
        bandwidth by 2^(CBS_CONDITION_SHIFT_AMOUNT). This is meant
        to avoid the possiblity of the integer division resulting in 0. 
        
        we exit without doing anything if Cs < (ds - rjob) * (Qs / T),
        which means the condition is not met. 
    */
    if(deadline > arrival){                                                         
        uint32_t budget = (cbs->budget.current << CONFIG_CBS_CONDITION_SHIFT_AMOUNT);                       
        uint32_t bandwidth = (cbs->budget.max << CONFIG_CBS_CONDITION_SHIFT_AMOUNT) / cbs->period;          
        if(budget < ((uint32_t)(deadline - arrival)) * bandwidth) return false;                        
    }

    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    cbs->abs_deadline = arrival + (uint64_t) cbs->period;
    #else
    cbs->abs_deadline = arrival + cbs->period;
    #endif
    
    cbs->budget.current = cbs->budget.max;
    k_thread_deadline_set(cbs->thread, (int) cbs->period);
    return true;
}


static void cbs_budget_timer_start(cbs_t *cbs){
    if(!cbs) return;
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    cbs->start_cycle = k_cycle_get_64();
    #else
    cbs->start_cycle = k_cycle_get_32();
    #endif
    k_timer_start(&cbs->timer, K_CYC(cbs->budget.current), K_NO_WAIT);
}


static void cbs_budget_timer_stop(cbs_t *cbs){
    if(cbs->timer.status) return;
    k_timer_stop(&cbs->timer);
}


void cbs_thread(void *job_queue, void *cbs_struct, void *unused){
    (void) unused;
    cbs_job_t job;
    cbs_t *cbs = (cbs_t *) cbs_struct;

	cbs->thread = k_current_get();
    cbs->thread->cbs = cbs;
    cbs->start_cycle = 0;
    cbs->queue = (struct k_msgq *) job_queue;
    k_timer_init(&cbs->timer, cbs_budget_ran_out, cbs_budget_stop);
    k_timer_user_data_set(&cbs->timer, cbs);

    for(;;){
        k_msgq_get(cbs->queue, &job, K_FOREVER);
        cbs_budget_timer_start(cbs);
        job.function(job.arg);
        cbs_budget_timer_stop(cbs);

        #ifdef CONFIG_CBS_LOG
        cbs_trace(CBS_COMPLETED_JOB, cbs);
        #endif
    }
}


void cbs_thread_switched_in(struct k_thread *thread){
    if(!thread || !thread->cbs) return;
    cbs_t *cbs = thread->cbs;
    cbs_budget_update_consumption(cbs);
    cbs_budget_timer_start(cbs);
    #ifdef CONFIG_CBS_LOG
    cbs_trace(CBS_SWITCH_TO, cbs);
    #endif
}


void cbs_thread_switched_out(struct k_thread *thread){
    if(!thread || !thread->cbs) return;
    cbs_t *cbs = thread->cbs;
    cbs_budget_timer_stop(cbs);
    #ifdef CONFIG_CBS_LOG
    cbs_trace(CBS_SWITCH_AWAY, cbs);
    #endif
}


void k_cbs_push_job(cbs_t *cbs, cbs_callback_t job_function, void *job_arg, k_timeout_t timeout){
    cbs_job_t job = { job_function, job_arg };
    if(!cbs) return;

    #ifdef CONFIG_CBS_LOG
    cbs_trace(CBS_PUSH_JOB, cbs);
    #endif

    #ifdef CONFIG_CBS_LOG
    bool condition_met = cbs_budget_restore_on_condition(cbs);
    if(condition_met) cbs_trace(CBS_BUDGET_CONDITION_MET, cbs);
    #else
    cbs_budget_restore_on_condition(cbs);
    #endif
    k_msgq_put(cbs->queue, &job, timeout);
}