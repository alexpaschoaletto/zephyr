#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

/*
	Remember that for "pure" EDF results, all
	user threads must have the same static priority.
	That happens because EDF will be a tie-breaker
	among two or more ready tasks of the same static
	priority. An arbitrary positive number is chosen here.
*/

#define EDF_PRIORITY            5
#define INACTIVE                -1
#define MSEC_TO_CYC(msec)       ((msec * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) / MSEC_PER_SEC)
#define MSEC_TO_USEC(msec)      (msec * 1000)

typedef struct {
    int id;
    int32_t rel_deadline_msec;
    int32_t period_msec;
    uint32_t wcet_msec;
    k_tid_t thread;
    struct k_msgq queue;
    struct k_timer timer;
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