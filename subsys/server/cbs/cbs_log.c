/* # ALEX adicionou isto
*  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
*  SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/logging/log.h>
#include <cbs_log.h>

LOG_MODULE_REGISTER(CBS);

void cbs_trace(enum cbs_evt event, cbs_t *cbs){
    switch(event){
        case CBS_PUSH_JOB:                                             // when a job is pushed to a server
            LOG_INF("%s\tJ_PUSH\t%u", (char *) cbs->name, cbs->budget.current);
        break;
        case CBS_SWITCH_TO:                                            // when cbs thread enters the CPU
            LOG_INF("%s\tSWT_TO\t%u", (char *) cbs->name, cbs->budget.current);
        break;
        case CBS_SWITCH_AWAY:                                          // when cbs thread leaves the CPU
            LOG_INF("%s\tSWT_AY\t%u", (char *) cbs->name, cbs->budget.current);
        break;
        case CBS_COMPLETED_JOB:                                        // when cbs completes the execution of a job
            LOG_INF("%s\tJ_COMP\t%u", (char *) cbs->name, cbs->budget.current);
        break;
        case CBS_BUDGET_RAN_OUT:                                       // when cbs budget runs out
            LOG_INF("%s\tB_ROUT\t%u", (char *) cbs->name, cbs->budget.current);
        break;
        case CBS_BUDGET_CONDITION_MET:                                 // when new job arrives and condition Cs >= (ds - rjob) * (Qs / T) is met
            LOG_INF("%s\tB_COND\t%u", (char *) cbs->name, cbs->budget.current);
        break;
    }
}