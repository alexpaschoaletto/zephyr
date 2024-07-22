#ifndef ZEPHYR_CBS_INTERNAL
#define ZEPHYR_CBS_INTERNAL

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

void cbs_thread_switched_in(struct k_thread *thread);
void cbs_thread_switched_out(struct k_thread *thread);
bool cbs_is_idle(struct k_thread *cbs_thread);
void cbs_thread(void *job_queue, void *cbs_struct, void *unused);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS_INTERNAL */