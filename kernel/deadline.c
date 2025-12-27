#include <zephyr/kernel.h>
#include <ksched.h>

#define HAS_PERIOD(thread)                              (thread->base.period > 0)
#define HAS_DEADLINE_CALLBACK(thread)                   (thread->base.deadline_miss_callback != NULL)
#define DEADLINE_IS_ZERO(deadline)                      (K_TIMEOUT_EQ(deadline, K_NO_WAIT))

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


int z_thread_deadline_set(k_tid_t thread, k_timeout_t deadline, bool is_absolute)
{
    int status = 0;
    int64_t now = k_uptime_ticks();

    if(DEADLINE_IS_ZERO(deadline))
    {
        /* thread is leaving the EDF scheduler */
        set_thread_deadline(thread, 0);
        thread->base.period_changed = false;
        thread->base.activation_tick = 0;
        return status;
    }

    if(HAS_PERIOD(thread))
    {
        /* if we're here, the task has configured a periodic execution. */
        if(thread->base.activation_tick == 0)
        {
            /* if activation_tick = 0, this is first cycle ever */
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

        thread->base.period_changed = false;
    }

    int absolute_deadline = (is_absolute)? deadline.ticks : (now + deadline.ticks);

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

    set_thread_deadline(thread, absolute_deadline);
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