#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

/*
	Remember that for "pure" EDF results, all
	user threads must have the same static priority.
	That happens because EDF will be a tie-breaker
	among two or more ready tasks of the same static
	priority. An arbitrary positive number is chosen here.
*/
#define EDF_PRIORITY 5

#define INACTIVE    -1

#define SEC_TO_CYC(sec)     (sec * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC)
#define USEC_TO_CYC(usec)   ((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / USEC_PER_SEC) * usec)

typedef struct {
    int32_t period_cyc;
    int32_t rel_deadline_cyc;
} edf_t;

typedef struct {
    char msg[100];
    uint32_t counter;
} job_t;

void report_cbs_settings(){
    printf("\n//////////////////////////////////////////////////////////////////////////////////////\n");
    printf("\nBoard:\t\t%s\n", CONFIG_BOARD_TARGET);
    #ifdef CONFIG_CBS
            printf("[CBS]\t\tCBS enabled.\n");
        #ifdef CONFIG_CBS_LOG
            printf("[CBS]\t\tCBS events logging: enabled.\n");
        #else
            printf("[CBS]\t\tCBS events logging: disabled.\n");
        #endif
        #ifdef CONFIG_TIMEOUT_64BIT
            printf("[CBS]\t\tSYS 64-bit timeouts: supported.\n");
        #else
            printf("[CBS]\t\tSYS 64-bit timeouts: not supported. using 32-bit API instead.\n");
        #endif
    #else
        printf("[CBS]\t\tCBS disabled.\n");
    #endif
    printf("\n//////////////////////////////////////////////////////////////////////////////////////\n\n");
}


#endif