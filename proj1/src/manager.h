/**
 * @file manager.h
 */
#ifndef _MANAGER_H
#define _MANAGER_H

#include "proc_structs.h"
#include "proc_gen.h"

typedef enum {PRIOR = 0, RR, FCFS} schedule_t;

typedef struct pcb_queue_t {
    struct pcb_t *first;
    struct pcb_t *last;
} pcb_queue_t;

/* --- Function Prototypes -------------------------------------------------- */

/** Initializes the manager. */
void init_queues(pcb_t *procs_loaded);

/**
 * Schedules processes.
 *
 * @param[in]  algorithm 
 * @param[in]  time_quantum
 */
void schedule_processes(schedule_t algorithm, int time_quantum);

/** Frees the manager. */
void free_manager(void);

#endif 
