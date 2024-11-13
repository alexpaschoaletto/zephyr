#ifndef _APP_TRACER_H_
#define _APP_TRACER_H_

#include <string.h>

#define TRIGGER         0
#define START           1
#define END             2 
#define BUF_SIZE        100

/*
    This struct will hold execution
    metadata for the threads
*/
typedef struct {
    uint32_t timestamp;
    char thread_id;
    int counter;
    int event;
} trace_t;


trace_t events[BUF_SIZE];
int event_count = 0;


void toString(int evt, char *target){
    switch(evt){
        case TRIGGER:
            strcpy(target, "TRIG ");
            break;
        case START:
            strcpy(target, "START");
            break;
        case END:
            strcpy(target, "END  ");
            break;
        default:
            strcpy(target, "-----");
    }
}


void reset_trace(){
    event_count = 0;
}


void print_trace(){
    printf("\n========================\nEDF events:\n");
    char event[10];
    for(int i = 0; i < event_count; i++){
        uint32_t timestamp = events[i].timestamp;
        toString(events[i].event, event);
        printf("%u  \t[ %c ] %s %d\n", timestamp, events[i].thread_id, event, events[i].counter);
    }
    printf("========================\n");
    reset_trace();
}


void trace(char thread_id, int thread_counter, int event){
    if(event_count >= BUF_SIZE) return;
    events[event_count].timestamp = k_uptime_get_32();
    events[event_count].thread_id = thread_id;
    events[event_count].counter = thread_counter;
    events[event_count].event = event;
    event_count++;
}


#endif