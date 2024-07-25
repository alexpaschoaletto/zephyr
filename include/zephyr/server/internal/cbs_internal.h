/* 
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#ifndef ZEPHYR_CBS_INTERNAL
#define ZEPHYR_CBS_INTERNAL

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
typedef uint64_t cbs_cycle_t;
#else
typedef uint32_t cbs_cycle_t;
#endif

void cbs_thread_switched_in(struct k_thread *thread);
void cbs_thread_switched_out(struct k_thread *thread);
bool cbs_is_idle(struct k_thread *cbs_thread);
void cbs_thread(void *job_queue, void *cbs_struct, void *unused);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS_INTERNAL */