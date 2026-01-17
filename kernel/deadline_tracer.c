/*
 * Copyright (c) 2026 Instituto Superior de Engenharia do Porto (ISEP).
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/kernel/deadline_tracer.h>
#include <string.h>

typedef struct {
	uint32_t timestamp;
	char thread_id;
	int counter;
	int event;
} trace_t;

typedef struct {
    uint32_t timestamp;
    char thread_id;
    int event;
    int64_t current_budget;
    int64_t deadline;
    int64_t cbs_budget;
    int64_t cbs_period;
} trace_cbs_t;

trace_t events[DEADLINE_TRACER_BUF_SIZE];
trace_cbs_t cbs_events[DEADLINE_TRACER_BUF_SIZE];

uint32_t offset;
int event_count;
int cbs_event_count;

void toString(int evt, char *target)
{
	switch (evt) {
        case TRIGGER:
            strcpy(target, "TRIG    ");
            break;
        case START:
            strcpy(target, "START   ");
            break;
        case END:
            strcpy(target, "END     ");
            break;
        case B_ROUT:
            strcpy(target, "B_ROUT  ");
            break;
        case B_COND:
            strcpy(target, "B_COND  ");
            break;
        case B_CHECK:
            strcpy(target, "B_CHECK ");
            break;
        case SWT_TO:
            strcpy(target, "SWT_TO  ");
            break;
        case SWT_AY:
            strcpy(target, "SWT_AY  ");
            break;
        default:
            strcpy(target, "--------");
	}
}

void reset_trace(void)
{
	event_count = 0;
    cbs_event_count = 0;
}

void begin_trace(void)
{
	offset = k_uptime_get_32();
	reset_trace();
}

void print_trace(void)
{
	char event[10];

	printf("\n========================\nEDF events:\n");

	for (int i = 0; i < event_count; i++) {
		uint32_t timestamp = events[i].timestamp - offset;

		toString(events[i].event, event);
		printf("%u  \t[ %c ] %s %d\n", timestamp, events[i].thread_id, event,
		       events[i].counter);
	}
    printf("========================\nCBS events:\n");
    for (int i = 0; i < cbs_event_count; i++) {
		uint32_t timestamp = cbs_events[i].timestamp - offset;

        toString(cbs_events[i].event, event);

        if(cbs_events[i].event == B_CHECK){
            /* for this event the entered deadline is relative */
            bool condition = (cbs_events[i].current_budget * cbs_events[i].cbs_period) > (cbs_events[i].deadline * cbs_events[i].cbs_budget);
            printf("%u  \t[ %c ] %s (%lld * %lld) > (%lld * %lld) --> %s\n", 
                timestamp, 
                cbs_events[i].thread_id,
                event,
                cbs_events[i].current_budget,
                cbs_events[i].cbs_budget,
                cbs_events[i].deadline,
                cbs_events[i].cbs_period,
                condition? "true" : "false"
            );
        } else {
            /* for this event the entered deadline is absolute */
            printf("%u  \t[ %c ] %s %lld\n", timestamp, cbs_events[i].thread_id, event,
		       cbs_events[i].deadline);
        }
	}
	printf("========================\n\n");
	reset_trace();
}

void trace(char thread_id, int thread_counter, int event)
{
	if (event_count >= DEADLINE_TRACER_BUF_SIZE) {
		return;
	}
	events[event_count].timestamp = k_uptime_get_32();
	events[event_count].thread_id = thread_id;
	events[event_count].counter = thread_counter;
	events[event_count].event = event;
	event_count++;
}

void trace_cbs(char thread_id, int64_t current_budget, int64_t deadline, int64_t cbs_budget, int64_t cbs_period, int event)
{
    if (cbs_event_count >= DEADLINE_TRACER_BUF_SIZE) {
		return;
	}
	cbs_events[cbs_event_count].timestamp = k_uptime_get_32();
	cbs_events[cbs_event_count].thread_id = thread_id;
    cbs_events[cbs_event_count].event = event;
	cbs_events[cbs_event_count].current_budget = current_budget;
	cbs_events[cbs_event_count].deadline = deadline;
    cbs_events[cbs_event_count].cbs_budget = cbs_budget;
    cbs_events[cbs_event_count].cbs_period = cbs_period;
	cbs_event_count++;
}


