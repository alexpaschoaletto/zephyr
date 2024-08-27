/* 
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#include <kthread.h>
#include <zephyr/server/internal/cbs_internal.h>
#include <zephyr/server/cbs.h>

bool cbs_is_idle(struct k_thread *cbs_thread){
    /*
        This has to be here because z_is_thread_pending
        is not visible to the subsys folder.
    */
    return z_is_thread_pending(cbs_thread);
}