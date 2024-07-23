/* 
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#include <kthread.h>
#include <zephyr/server/internal/cbs_internal.h>
#include <zephyr/server/cbs.h>

bool cbs_is_idle(struct k_thread *cbs_thread){
    return z_is_thread_pending(cbs_thread);
}