/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/kernel/deadline.h>
#include <zephyr/kernel/deadline_tracer.h>
#include <zephyr/kernel/deadline_tracer_overhead.h>
#include <ksched.h>

extern void set_thread_deadline(k_tid_t thread, int deadline);

void k_thread_period_set(k_tid_t thread, k_timeout_t period)
{
    if (thread->base.period == period.ticks) return;
    /*
        Note that this function can be called during
        task setup (normal case), but nothing stops it
        from changing its own period mid-execution.
        If that happens, we have two possibilities:
        
        1. the new period is greater than the last. If that
        happens, all good. there's nothing special to do.
        
        2.  the new period is smaller than the last. In this
        case, there is a potential false alarm that the task
        has overflown the previous period if the new period
        is less than the last execution time.

        Thus, we need to know that the period has been
        re-adjusted mid-execution to avoid the false alarm.
        the simplest way of doing so is to have a "changed"
        flag in the thread control block, and to only check
        for overflows if changed == false.
    */
    thread->base.period = (int64_t) period.ticks;
    thread->base.period_changed = true;
}


void k_thread_deadline_miss_callback_set(k_tid_t thread, void (* callback)(void *), void *callback_arg)
{
   thread->base.deadline_miss_callback = callback;
   k_timer_user_data_set(&(thread->base.deadline_timer), callback_arg);
}


void z_deadline_timer_callback(struct k_timer *deadline_timer)
{
    struct k_thread *thread = CONTAINER_OF(deadline_timer, struct k_thread, base.deadline_timer);

    if(thread->base.deadline_miss_callback == NULL) return;
    thread->base.deadline_miss_callback(k_timer_user_data_get(deadline_timer));
}


// CBS callbacks //////////////////////////////////////////////////////////////////////////////////////

void k_thread_cbs_budget_set(k_tid_t thread, k_timeout_t budget)
{
    if(budget.ticks == 0) {
        /* task is disabling the CBS */
        k_timer_stop(&(thread->base.cbs.budget_timer));
    }
    thread->base.cbs.max_budget = budget.ticks;
}


static inline bool z_cbs_check_condition(struct k_thread *thread)
{
    int64_t now = k_uptime_ticks();
    int64_t absolute_deadline = thread->base.prio_deadline;

    struct k_cbs *cbs = &(thread->base.cbs);
    /*
     * the CBS condition is a question: is my current bandwidth
     * (current budget / time until absolute deadline) higher
     * than the maximum allowed (max budget / relative deadline)?
     * 
     * If true, we have to intervene so that the CBS task
     * never exceeds its maximum bandwidth.
     */
    // trace_cbs(' ', cbs->current_budget, (absolute_deadline - now), cbs->max_budget, cbs->relative_deadline, B_CHECK);
    return (
        (cbs->current_budget * cbs->relative_deadline) > (cbs->max_budget * (absolute_deadline - now))
    );
}


void z_cbs_switched_in(struct k_cbs *cbs)
{
    // trace_overhead_start();	                                        /* OVERHEAD MEASUREMENT */
	/*
	 * this function is called at every context switch
	 * and just starts the budget timer if the incoming
	 * thread belongs to an active CBS. If it doesn't,
	 * there's nothing to do.
	 */
    k_timer_start(&(cbs->budget_timer), K_TICKS(cbs->current_budget), K_NO_WAIT);
    cbs->start_tick = k_uptime_ticks();
    // trace(' ', cbs->current_budget, SWT_TO);

	// trace_switched_to_overhead(cbs_get_now() - over_start); 	    /* OVERHEAD MEASUREMENT */
}


void z_cbs_switched_out(struct k_cbs *cbs)
{
    // trace_overhead_start();	                                        /* OVERHEAD MEASUREMENT */
	/*
	 * this function is called at every context switch
	 * and just stops the budget timer if the outgoing
	 * thread belongs to an active CBS. If it doesn't,
	 * there's nothing to do.
	 */
    k_timer_stop(&(cbs->budget_timer));
    cbs->current_budget -= (k_uptime_ticks() - cbs->start_tick);  
    cbs->start_tick = 0;
    // trace(' ', cbs->current_budget, SWT_AY);

	// trace_switched_away_overhead(cbs_get_now() - over_start); 		/* OVERHEAD MEASUREMENT */
}


/*
 * this callback runs when the execution budget is exhausted.
 * the CBS is opt-in just like the deadline miss callback and
 * the period, so it must also be configured by the task.
 * 
 * CBS enabling/disabling is done through invoking k_thread_cbs_budget_set.
 * That's the only value the application needs to provide, as the
 * other properties are inherited from the EDF default configuration.
 */
void z_cbs_timer_callback(struct k_timer *budget_timer)
{
    // trace_overhead_start();	                               /* OVERHEAD MEASUREMENT */

    int64_t now = k_uptime_ticks();
    struct k_cbs *cbs = CONTAINER_OF(budget_timer, struct k_cbs, budget_timer);
    struct k_thread *thread = cbs->thread;

    cbs->start_tick = now;
    cbs->current_budget = cbs->max_budget;
    int new_deadline = (int) (thread->base.prio_deadline + cbs->relative_deadline);

    k_timer_start(budget_timer, K_TICKS(cbs->max_budget), K_FOREVER);

    if(HAS_DEADLINE_CALLBACK(thread))
    {
        k_timer_start(&(thread->base.deadline_timer), K_TICKS(new_deadline - now), K_FOREVER);
    }

    trace_cbs(' ', 0, new_deadline, 0, 0, B_ROUT);
    set_thread_deadline(thread, new_deadline);
    k_reschedule();

	// trace_timer_overhead(cbs_get_now() - over_start);	    /* OVERHEAD MEASUREMENT */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int z_thread_deadline_set(k_tid_t thread, k_timeout_t deadline, bool is_absolute)
{
    int status = 0;
    int64_t now = k_uptime_ticks();

    if(DEADLINE_IS_ZERO(deadline))
    {
        /* thread is leaving the EDF scheduler */
        thread->base.period_changed = false;
        thread->base.activation_tick = 0;
        set_thread_deadline(thread, 0);
        return status;
    }

    if(HAS_PERIOD(thread))
    {
        /* if we're here, the task has configured a periodic execution. */
        if(IS_FIRST_CYCLE(thread))
        {
            thread->base.activation_tick = now;
        }
        else
        {
            /*
             * if activation_tick != 0, we have just requested the
             * start of a new period (i.e. the last one is technically
             * still running). Thus we need to stop the watchdog
             * timer, if any, and check if we need to wait or not
             * for the last period to finish before actually starting
             * the requested one.
             */
            if(HAS_DEADLINE_CALLBACK(thread))
            {
                k_timer_stop(&(thread->base.deadline_timer));
            }

            if(HAS_CBS_ACTIVE(thread))
            {
                struct k_cbs *cbs = &(thread->base.cbs);
                k_timer_stop(&(cbs->budget_timer));
                cbs->current_budget -= (now - cbs->start_tick);  
                cbs->start_tick = 0;
                cbs->is_active = false;
            }

            int64_t next_activation = thread->base.activation_tick + thread->base.period;

            if(now > next_activation)
            {
                if(thread->base.period_changed == false)
                {
                    /*
                     * execution exceeded the period AND there was no
                     * change in the period itself during the last cycle.
                     * Although undesired, this is not necessarily critical
                     * in EDF because deadlines can differ from periods;
                     * Thus, a warning might be enough.
                     * 
                     * We consider the application knows what it's doing if
                     * it purposefully set the new period to be smaller than
                     * its last execution cycle.
                     */
                    status = -1;
                }
            }
            else
            {
                /*
                 * execution has gone within period (as expected),
                 * so we can just sleep for for the remainder of
                 * the current period before starting the next
                 */
                k_sleep(K_TICKS(next_activation - now));

                /*
                 * we have just slept for a while, so the previous
                 * "now" is outdated. We have to update it
                 * 
                 * we DON'T want to do now = k_uptime_ticks() here
                 * to prevent timedrift as the cycles add up.
                 */
                now = next_activation;
            }

            thread->base.activation_tick = now;
        }

        /* unconditionally reset the "period changed mid-cycle" flag */
        thread->base.period_changed = false;
    }

    int absolute_deadline = (is_absolute)? deadline.ticks : (now + deadline.ticks);

    if(HAS_CBS_ENABLED(thread))
    {
        /*
         * thread has declared an execution budget, so
         * it's regulated by a CBS. When this option
         * is enabled, calling k_thread_deadline_set
         * is seen as pushing a job to the CBS.
         * 
         * Thus, a new deadline is set if and only if
         * the bandwidth condition is met.
         */
        struct k_cbs *cbs = &(thread->base.cbs);
        cbs->is_active = true;
        cbs->start_tick = now;
        cbs->relative_deadline = (is_absolute)? (absolute_deadline - now) : deadline.ticks;

        if(z_cbs_check_condition(thread) == true)
        {
            /*
             * if we're here, the condition is met.
             * We intervene by setting a new deadline
             * and replenishing the current budget
             * to its maximum value.
             */
            cbs->current_budget = cbs->max_budget;
            set_thread_deadline(thread, absolute_deadline);
            trace_cbs(' ', 0, absolute_deadline, 0, 0, B_COND);
        }
        else if(HAS_DEADLINE_CALLBACK(thread))
        {
            /*
             * if condition is NOT met, then we may serve the
             * next cycle with the same deadline and budget as
             * before.
             * 
             * In case the deadline callback is configured,
             * we need to override the provided deadline to
             * make it match the current thread's deadline.
             */
            is_absolute = true;
            deadline.ticks = thread->base.prio_deadline;
        }

        k_timer_start(&(cbs->budget_timer), K_TICKS(cbs->current_budget), K_FOREVER);
    }
    else
    {
        /*
        * This is the default case: normal EDF, with
        * no CBS configured. If so, we unconditionally
        * set the deadline.
        */
        set_thread_deadline(thread, absolute_deadline);
    }

    if (HAS_DEADLINE_CALLBACK(thread))
    {
        /* task has declared a callback for an eventual deadline miss */
        if ( is_absolute && ( deadline.ticks > now ))
        {
            k_timeout_t relative_deadline = K_TICKS(deadline.ticks - now);
            k_timer_start(&(thread->base.deadline_timer), relative_deadline, K_FOREVER);
        }
        else
        {
            k_timer_start(&(thread->base.deadline_timer), deadline, K_FOREVER);
        }
    }

    return status;
}

int z_impl_k_thread_absolute_deadline_set(k_tid_t thread, k_timeout_t deadline)
{
	return z_thread_deadline_set(thread, deadline, true);
}

int z_impl_k_thread_deadline_set(k_tid_t thread, k_timeout_t deadline)
{
	return z_thread_deadline_set(thread, deadline, false);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_thread_absolute_deadline_set(k_tid_t thread, k_timeout_t deadline)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));

	z_impl_k_thread_absolute_deadline_set(thread, deadline);
}
#include <zephyr/syscalls/k_thread_absolute_deadline_set_mrsh.c>

static inline void z_vrfy_k_thread_deadline_set(k_tid_t thread, k_timeout_t deadline)
{
	K_OOPS(K_SYSCALL_OBJ(thread, K_OBJ_THREAD));
	K_OOPS(K_SYSCALL_VERIFY_MSG(deadline.ticks > 0,
				    "invalid thread deadline %d",
				    (int)deadline));

	z_impl_k_thread_deadline_set(thread, deadline);
}
#include <zephyr/syscalls/k_thread_deadline_set_mrsh.c>
#endif /* CONFIG_USERSPACE */
