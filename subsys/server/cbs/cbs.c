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

static cbs_cycle_t cbs_get_now(){
    #ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
    return k_cycle_get_64();
    #else
    return k_cycle_get_32();
    #endif
}


static void cbs_replenish_on_condition(cbs_t *cbs, cbs_cycle_t cycle){
    cbs->abs_deadline = cycle + cbs->period;
    cbs->budget.current = cbs->budget.max;

    #ifdef CONFIG_CBS_LOG
    cbs_log(CBS_BUDGET_CONDITION_MET, cbs);
    #endif
}


static void cbs_replenish_on_run_out(cbs_t *cbs, cbs_cycle_t cycle){
    cbs->start_cycle = cycle;
    cbs->abs_deadline += cbs->period;
    cbs->budget.current = cbs->budget.max;

    #ifdef CONFIG_CBS_LOG
    cbs_log(CBS_BUDGET_RAN_OUT, cbs);
    #endif
}


static void cbs_budget_update_consumption(cbs_t *cbs){
    if(!cbs || !cbs->start_cycle) return;
    cbs_cycle_t now = cbs_get_now();
    cbs_cycle_t budget_used = now - cbs->start_cycle;
    if(budget_used < cbs->budget.current){
        /*
            This is the expected case when this function
            is called: we have used less budget than what
            we have available. A simple subtracion will
            take care of updating what's left.
        */
        cbs->budget.current -= budget_used;
        return;
    }
    /*
        If we end up here, the job has finished before
        the timer expired but also spent more than
        the allowed budget. We therefore need to
        compensate that on replenishing.
    */
    cbs_cycle_t excess;
    if(budget_used > cbs->budget.max) {
        /*
            Very edge case where the job surpassed
            the max allowed value for the budget
            itself one or more times in a row.
        */
        for(; budget_used > cbs->budget.max; budget_used -= cbs->budget.max){
            cbs_replenish_on_run_out(cbs, cbs->start_cycle + cbs->budget.max);
        }
        excess = budget_used;
    } else {
        excess = budget_used - cbs->budget.current;
    }
    cbs_replenish_on_run_out(cbs, (now - excess));
    k_thread_deadline_set(cbs->thread, (int) (cbs->abs_deadline - cbs->start_cycle));
    cbs->budget.current -= excess;
}


static void cbs_budget_timer_expired_callback(struct k_timer *timer){
    /*
        This function is called by the timer when it expires,
        which means the budget has been entirely used and
        thererefore needs replenishing.
    */
    if(!timer) return;
    cbs_cycle_t now = cbs_get_now();   
    cbs_t *cbs = (cbs_t *) k_timer_user_data_get(timer);
    cbs_replenish_on_run_out(cbs, now);
    k_thread_deadline_set(cbs->thread, (int) (cbs->abs_deadline - cbs->start_cycle));
    k_timer_start(timer, K_CYC((uint32_t) cbs->budget.current), K_NO_WAIT);
}


static void cbs_budget_timer_stop_callback(struct k_timer *timer){
    /*
        This function is called by the timer when it is stopped
        before expiring, which means the thread has finished
        executing the job or was preempted. There is still
        budget left, which is why we need to update its value.
    */
    if(!timer) return;
    cbs_t *cbs = (cbs_t *) k_timer_user_data_get(timer);
    cbs_budget_update_consumption(cbs);
    cbs->start_cycle = 0;
}


static void cbs_budget_restore_on_condition(cbs_t *cbs){
    if(!cbs_is_idle(cbs->thread)) return;
    cbs_cycle_t arrival = cbs_get_now();
    cbs_cycle_t deadline = cbs->abs_deadline;
    /*
        The CBS condition is that the server must be idle when a
        new job comes AND the following must hold:

        Cs >= (ds - rjob) * (Qs / T)

        Where Cs is the budget left, ds is the absolute deadline,
        rjob is the arrival instant of the job, Qs is the server
        max budget and T is the server period.

        So if arrival >= deadline, condition is instantly met. Otherwise,
        we need to check more stuff. Note we multiply both budget and
        bandwidth by 2^(CBS_CONDITION_SHIFT_AMOUNT). This is meant
        to avoid the possiblity of the integer division resulting in 0. 
        
        we exit without doing anything if Cs < (ds - rjob) * (Qs / T),
        which means the condition is not met. 
    */
    if(deadline > arrival){                                                         
        cbs_cycle_t budget = (cbs->budget.current << CONFIG_CBS_CONDITION_SHIFT_AMOUNT);                       
        cbs_cycle_t bandwidth = (cbs->budget.max << CONFIG_CBS_CONDITION_SHIFT_AMOUNT) / cbs->period;          
        if(budget < (deadline - arrival) * bandwidth) return;                        
    }
    cbs_replenish_on_condition(cbs, arrival);
    k_thread_deadline_set(cbs->thread, (int) cbs->period);
}


static void cbs_budget_timer_start(cbs_t *cbs){
    if(!cbs) return;
    cbs->start_cycle = cbs_get_now();
    k_timer_start(cbs->timer, K_CYC((uint32_t) cbs->budget.current), K_NO_WAIT);
}


static void cbs_budget_timer_stop(cbs_t *cbs){
    if(!cbs || cbs->timer->status) return;
    k_timer_stop(cbs->timer);
}


void cbs_thread(void *server_name, void *cbs_struct, void *cbs_timer){
    cbs_job_t job;
    cbs_t *cbs = (cbs_t *) cbs_struct;
    struct k_timer *timer = (struct k_timer *) cbs_timer;

	cbs->thread = k_current_get();
    cbs->thread->cbs = cbs;
    cbs->start_cycle = 0;
    cbs->timer = timer;
    k_timer_init(cbs->timer, cbs_budget_timer_expired_callback, cbs_budget_timer_stop_callback);
    k_timer_user_data_set(cbs->timer, cbs);

    #ifdef CONFIG_CBS_LOG
    strncpy(cbs->name, (char *) server_name, CONFIG_CBS_THREAD_MAX_NAME_LEN - 1);
    #endif

    for(;;){
        k_msgq_get(cbs->queue, &job, K_FOREVER);
        cbs_budget_timer_start(cbs);
        job.function(job.arg);
        cbs_budget_timer_stop(cbs);

        #ifdef CONFIG_CBS_LOG
        cbs_log(CBS_COMPLETED_JOB, cbs);
        #endif
    }
}


void cbs_thread_switched_in(struct k_thread *thread){
    if(!thread || !thread->cbs) return;
    cbs_t *cbs = (cbs_t *)thread->cbs;
    cbs_budget_timer_start(cbs);

    #ifdef CONFIG_CBS_LOG
    cbs_log(CBS_SWITCH_TO, cbs);
    #endif
}


void cbs_thread_switched_out(struct k_thread *thread){
    if(!thread || !thread->cbs) return;
    cbs_t *cbs = (cbs_t *)thread->cbs;
    cbs_budget_timer_stop(cbs);

    #ifdef CONFIG_CBS_LOG
    cbs_log(CBS_SWITCH_AWAY, cbs);
    #endif
}


void k_cbs_push_job(cbs_t *cbs, cbs_callback_t job_function, void *job_arg, k_timeout_t timeout){
    cbs_job_t job = { job_function, job_arg };
    if(!cbs) return;

    #ifdef CONFIG_CBS_LOG
    cbs_log(CBS_PUSH_JOB, cbs);
    #endif

    cbs_budget_restore_on_condition(cbs);
    k_msgq_put(cbs->queue, &job, timeout);
}