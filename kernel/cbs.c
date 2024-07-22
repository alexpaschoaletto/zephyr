#include <kthread.h>
#include <zephyr/server/internal/cbs_internal.h>
#include <zephyr/server/cbs.h>

bool cbs_is_idle(struct k_thread *cbs_thread){
    return z_is_thread_pending(cbs_thread);
}