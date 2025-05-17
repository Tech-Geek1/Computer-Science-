/**
 * @mainpage Process Simulation
 *
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "proc_structs.h"
#include "proc_syntax.h"
#include "logger.h"
#include "manager.h"

#define LOWEST_PRIORITY -1 // Use INT_MAX if 0 has the highest priority

int num_processes = 0;

/**
 * The queues as required by the spec
 */
static pcb_queue_t terminatedq;
static pcb_queue_t waitingq;
static pcb_queue_t readyq;
static bool_t readyq_updated;

void schedule_fcfs();
void schedule_rr(int quantum);
void schedule_pri_w_pre();
bool_t higher_priority(int, int);

void execute_instr(pcb_t *proc, instr_t *instr);
void request_resource(pcb_t *proc, instr_t *instr);
void release_resource(pcb_t *proc, instr_t *instr);
bool_t acquire_resource(pcb_t *proc, char *resource_name);

void check_for_new_arrivals();
int get_algo(int num_args, char **argv);
int get_time_quantum(int num_args, char **argv);
void print_args(char *data1, char *data2, int sched, int tq);

void move_proc_to_wq(pcb_t *pcb, char *resource_name);
void move_waiting_pcbs_to_rq(char *resource_name);
void move_proc_to_rq(pcb_t *pcb);
void move_proc_to_tq(pcb_t *pcb);
void enqueue_pcb(pcb_t *proc, pcb_queue_t *queue);
pcb_t *dequeue_pcb(pcb_queue_t *queue);

char *get_init_data(int num_args, char **argv);
char *get_data(int num_args, char **argv);
int get_algo(int num_args, char **argv);
int get_time_quantum(int num_args, char **argv);
void print_args(char *data1, char *data2, int sched, int tq);

void print_avail_resources(void);
void print_alloc_resources(pcb_t *proc);
void print_queue(pcb_queue_t queue, char *msg);
void print_running(pcb_t *proc, char *msg);
void print_instructions(instr_t *instr);

int main(int argc, char **argv)
{
    char *data1 = get_init_data(argc, argv);
    char *data2 = get_data(argc, argv);
    int scheduler = get_algo(argc, argv);
    int time_quantum = get_time_quantum(argc, argv);
    print_args(data1, data2, scheduler, time_quantum);

    pcb_t *initial_procs = NULL;
    if (strcmp(data1, "generate") == 0)
    {
#ifdef DEBUG_MNGR
        printf("****Generate processes and initialise the system\n");
#endif
        initial_procs = init_loader_from_generator();
    }
    else
    {
#ifdef DEBUG_MNGR
        printf("Parse process files and initialise the system: %s, %s \n", data1, data2);
#endif
        initial_procs = init_loader_from_files(data1, data2);
    }

    /* schedule the processes */
    if (initial_procs)
    {
        num_processes = get_num_procs();
        init_queues(initial_procs);
        printf("***********Scheduling processes************\n");
        schedule_processes(scheduler, time_quantum);
        dealloc_data_structures();
    }
    else
    {
        printf("Error: no processes to schedule\n");
    }

    return EXIT_SUCCESS;
}
/**
 * @brief The linked list of loaded processes is moved to the readyqueue.
 *        The waiting and terminated queues are intialised to empty
 * @param cur_pcb: a pointer to the linked list of loaded processes
 */
void init_queues(pcb_t *cur_pcb)
{
    readyq.first = cur_pcb;
    for (cur_pcb = readyq.first; cur_pcb->next != NULL; cur_pcb = cur_pcb->next)
        ;
    readyq.last = cur_pcb;
    readyq_updated = FALSE;

    waitingq.last = NULL;
    waitingq.first = NULL;
    terminatedq.last = NULL;
    terminatedq.first = NULL;

#ifdef DEBUG_MNGR
    printf("-----------------------------------");
    print_queue(readyq, "Ready");
    printf("\n-----------------------------------");
    print_queue(waitingq, "Waiting");
    printf("\n-----------------------------------");
    print_queue(terminatedq, "Terminated");
    printf("\n");
#endif /* DEBUG_MNGR */
}

/**
 * @brief Schedules each instruction of each process
 *
 * @param type The scheduling algorithm to use
 * @param quantum The time quantum for the RR algorithm, if used.
 */
void schedule_processes(schedule_t sched_type, int quantum)
{
    switch (sched_type)
    {
    case PRIOR:
        schedule_pri_w_pre();
        break;
    case RR:
        schedule_rr(quantum);
        break;
    case FCFS:
        schedule_fcfs();
        break;
    default:
        break;
    }
}

/**
 * Schedules processes using priority scheduling with preemption
 */
void schedule_pri_w_pre()
{

    while (readyq.first != NULL)
    {
        pcb_t *current_process = readyq.first;
        pcb_t *highest_priority_process = current_process;

        // Find the highest priority process
        while (current_process != NULL)
        {
            if (current_process->priority > highest_priority_process->priority)
            {
                highest_priority_process = current_process;
            }
            current_process = current_process->next;
        }

        // Remove the highest priority process from the ready queue
        if (highest_priority_process == readyq.first)
        {
            readyq.first = highest_priority_process->next;
            if (readyq.first == NULL)
            {
                readyq.last = NULL;
            }
        }
        else
        {
            pcb_t *prev = readyq.first;
            while (prev->next != highest_priority_process)
            {
                prev = prev->next;
            }
            prev->next = highest_priority_process->next;
            if (prev->next == NULL)
            {
                readyq.last = prev;
            }
        }

        // Execute instructions of the highest priority process
        while (highest_priority_process->next_instruction != NULL)
        {
            execute_instr(highest_priority_process, highest_priority_process->next_instruction);

            if (highest_priority_process->state == WAITING)
            {

                break;
            }

            check_for_new_arrivals();
            highest_priority_process->next_instruction = highest_priority_process->next_instruction->next;

            if (readyq.first != NULL && readyq.last->priority > highest_priority_process->priority)
            {

                move_proc_to_rq(highest_priority_process);
                highest_priority_process = readyq.last;

                break;
            }
        }

        // Move the process to terminated queue if all instructions are completed
        if (highest_priority_process->next_instruction == NULL)
        {

            highest_priority_process->state = TERMINATED;
            move_proc_to_tq(highest_priority_process);
        }
    }
}

/** Schedules processes using FCFS scheduling */
void schedule_fcfs()
{

    pcb_t *current_process = readyq.first;

    while (current_process != NULL)
    {
        // Set the state of the current process to RUNNING
        current_process->state = RUNNING;

        // Dequeue the current process from the ready queue
        dequeue_pcb(&readyq);

        while (current_process->next_instruction != NULL)
        {

            // Execute the next instruction of the current process
            execute_instr(current_process, current_process->next_instruction);
            check_for_new_arrivals();

            // Move to the next instruction of the current process
            current_process->next_instruction = current_process->next_instruction->next;
        }

        // Move the completed process to the terminated queue
        current_process->state = TERMINATED;
        move_proc_to_tq(current_process);

        // Get the next process from the ready queue
        current_process = readyq.first;
    }
}

/**
 * Schedules processes using the Round-Robin scheduler.
 *
 * @param[in] quantum time quantum
 */
void schedule_rr(int quantum)
{
    /* TODO: Implement if chosen as 2nd option */
}

/**
 * Executes a process instruction.
 *
 * @param[in] pcb
 *     processs for which to execute the instruction
 * @param[in] instr
 *     instruction to execute
 */
void execute_instr(pcb_t *pcb, instr_t *instr)
{

    if (instr != NULL)
    {
        switch (instr->type)
        {
        case REQ_OP:
            request_resource(pcb, instr);
            break;
        case REL_OP:

            release_resource(pcb, instr);
            break;
        default:
            break;
        }
    }
    else
    {
        printf("Error: No instruction to execute\n");
    }

#ifdef DEBUG_MNGR
    printf("-----------------------------------");
    print_running(pcb, "Running");
    printf("\n-----------------------------------");
    print_queue(readyq, "Ready");
    printf("\n-----------------------------------");
    print_queue(waitingq, "Waiting");
    printf("\n-----------------------------------");
    print_queue(terminatedq, "Terminated");
    printf("\n");
#endif
}

/**
 * @brief Handles the request resource instruction.
 *
 * Executes the request instruction for the process. The function loops
 * through the list of resources and acquires the resource if it is available.
 * If the resource is not available the process is moved to the waiting queue
 *
 * @param current The current process for which the resource must be acquired.
 * @param instruct The request instruction
 */
void request_resource(pcb_t *cur_pcb, instr_t *instr)
{

    resource_t *resource = get_available_resources();

    // Iterate through available resources
    while (resource != NULL)
    {
        // Check if resource name matches
        if (strcmp(resource->name, instr->resource_name) == 0)
        {
            if (resource->available == YES)
            {
                // Allocate memory for a copy of the resource
                resource_t *copy = (resource_t *)malloc(sizeof(resource_t));

                // Copy resource data
                copy->name = resource->name;
                copy->next = NULL;
                copy->available = NO;

                // Update resource availability
                resource->available = NO;

                // Add the copied resource to the pcb's resources
                if (cur_pcb->resources == NULL)
                {
                    cur_pcb->resources = copy;
                }
                else
                {
                    // Find the last resource in the PCB's resources list
                    resource_t *last = cur_pcb->resources;
                    while (last->next != NULL)
                    {
                        last = last->next;
                    }
                    // Append the new resource
                    last->next = copy;
                }

                // Log resource acquisition
                log_request_acquired(cur_pcb->process_in_mem->name, instr->resource_name);
                return;
            }
            else
            {

                // Check if process already has the resource
                resource_t *proc_resources = cur_pcb->resources;
                bool_t *proc_has_resource = false;

                while (proc_resources != NULL)
                {
                    if (strcmp(proc_resources->name, instr->resource_name) == 0)
                    {
                        proc_has_resource = true;
                        break;
                    }
                    proc_resources = proc_resources->next;
                }

                if (!proc_has_resource)
                {
                    // If resource not available, move process to waiting queue
                    cur_pcb->state = WAITING;
                    move_proc_to_wq(cur_pcb, resource->name);
                }
                else
                {
                    log_request_acquired(cur_pcb->process_in_mem->name, instr->resource_name);
                    return;
                }
            }
            return; // Exit loop after processing resource
        }

        // Move to the next resource
        resource = resource->next;
    }
}

/**
 * @brief Acquires a resource for a process.
 *
 * @param[in] process
 *     process for which to acquire the resource
 * @param[in] resource
 *     resource name
 * @return TRUE if the resourlog_errorce was successfully acquire_resource; FALSE otherwise
 */
bool_t acquire_resource(pcb_t *cur_pcb, char *resource_name)
{

    struct resource_t *resource = get_available_resources();

    // Check if the resource is available
    if (resource->available == YES)
    {

        cur_pcb->resources = resource;
        strcpy(resource->name, resource_name);

        // Set the availability of the resource to unavailable
        resource->available = NO;

        return TRUE;
    }

    // If the resource is not available, return FALSE
    return FALSE;
}

/**
 * @brief Handles the release resource instruction.
 *
 * Executes the release instruction for the process. If the resource can be
 * released the process is ready for next execution. If the resource can not
 * be released the process waits
 *
 * @param current The process which releases the resource.
 * @param instruct The instruction to release the resource.
 */
void release_resource(pcb_t *pcb, instr_t *instr)
{
    struct resource_t *available_resource = get_available_resources();

    // Iterate through the list of available resources
    while (available_resource != NULL)
    {

        if (strcmp(available_resource->name, instr->resource_name) == 0)
        {

            available_resource->available = YES;

            // Log the release of the resource
            log_release_released(pcb->process_in_mem->name, instr->resource_name);

            // Move processes waiting for resource back to the ready queue
            move_waiting_pcbs_to_rq(instr->resource_name);

            return;
        }
        // Move to the next available resource
        available_resource = available_resource->next;
    }
}

/**
 * Add new process <code>pcb</code> to ready queue
 */
void check_for_new_arrivals()
{
    pcb_t *new_pcb = get_new_pcb();

    if (new_pcb)
    {
        printf("New process arriving: %s\n", new_pcb->process_in_mem->name);
        move_proc_to_rq(new_pcb);
    }
}

/**
 * @brief Move process <code>pcb</code> to the ready queue
 *
 * @param[in] pcb
 */
void move_proc_to_rq(pcb_t *pcb)
{
    // Set the state of the process to READY
    pcb->state = READY;

    // Add the process into the ready queue
    enqueue_pcb(pcb, &readyq);

    // Log the readiness of the process
    log_request_ready(pcb->process_in_mem->name);
}

/**
 * Move process <code>pcb</code> to waiting queue
 */
void move_proc_to_wq(pcb_t *pcb, char *resource_name)
{

    // Set the state of the process to WAITING
    pcb->state = WAITING;

    // enqueue the process into the waiting queue
    enqueue_pcb(pcb, &waitingq);

    // Log the movement of the process
    log_request_waiting(pcb->process_in_mem->name, resource_name);
}

/**
 * Move process <code>pcb</code> to terminated queue
 *
 * @param[in] pcb
 */
void move_proc_to_tq(pcb_t *pcb)
{
    // Set the state of the process to TERMINATED
    pcb->state = TERMINATED;

    // Check if the terminated queue is empty
    if (terminatedq.first == NULL)
    {

        terminatedq.first = pcb;
        terminatedq.last = pcb;
        pcb->next = NULL;
    }
    else
    {
        // If the queue is not empty, add the current process to the end of the queue
        terminatedq.last->next = pcb;
        terminatedq.last = pcb;
        pcb->next = NULL;
    }

    // Log the termination of the process
    log_terminated(pcb->process_in_mem->name);
}

/**
 * Moves all processes waiting for resource <code>resource_name</code>
 * from the waiting queue to the readyq queue.
 *
 * @param[in]   resource
 *     resource name
 */
void move_waiting_pcbs_to_rq(char *resource_name)
{

    pcb_t *current_process = waitingq.first;
    pcb_t *prev_process = NULL;

    while (current_process != NULL)
    {
        // Get the next instruction of the current process
        instr_t *current_instruction = current_process->next_instruction;

        if (strcmp(current_instruction->resource_name, resource_name) == 0)
        {
            // If the resource matches, remove the process from the waiting queue
            if (prev_process != NULL)
            {
                prev_process->next = current_process->next;
            }
            else
            {
                waitingq.first = current_process->next;
            }

            // Move the process to the ready queue
            current_process->state = READY;
            enqueue_pcb(current_process, &readyq);

            // Log the movement of the process
            log_request_ready(current_process->process_in_mem->name);

            pcb_t *temp = current_process;
            current_process = current_process->next;
        }
        // Move to the next instruction of the current process
        current_instruction = current_instruction->next;

        if (current_instruction == NULL)
        {

            prev_process = current_process;
            current_process = current_process->next;
        }
    }
}

/**
 * Enqueues process <code>pcb</code> to <code>queue</code>.
 *
 * @param[in] process
 *     process to enqueue
 * @param[in] queue
 *     queue to which the process must be enqueued
 */
void enqueue_pcb(pcb_t *pcb, pcb_queue_t *queue)
{

    if (queue->first == NULL)
    {
        queue->first = pcb;
        queue->last = pcb;
        pcb->next = NULL;
    }
    else
    {
        queue->last->next = pcb;
        queue->last = pcb;
        pcb->next = NULL;
    }
}

/**
 * Dequeues a process from queue <code>queue</code>.
 *
 * @param[in] queue
 *     queue from which to dequeue a process
 * @return dequeued process
 */
pcb_t *dequeue_pcb(pcb_queue_t *queue)
{

    pcb_t *dequeued_process = queue->first;

    /* if queue is empty */
    if (queue->first == NULL)
    {
        return NULL;
    }
    else
    {

        queue->first = queue->first->next;

        if (queue->first == NULL)
        {

            queue->last = NULL;
        }
    }

    return dequeued_process;
}

/** @brief Return TRUE if pri1 has a higher priority than pri2
 *         where higher values == higher priorities
 *
 * @param[in] pri1
 *     priority value 1
 * @param[in] pri2
 *     priority value 2
 */
bool_t higher_priority(int pri1, int pri2)
{

    if (pri1 > pri2)
    {

        return TRUE;
    }
    else if (pri1 < pri2)
    {

        return FALSE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Inspect the waiting queue and detects deadlock
 */
struct pcb_t *detect_deadlock()
{
    printf("Function detect_deadlock not implemented\n");
    return NULL;
}

/**
 * @brief Releases a processes' resources and sets it to its first instruction.
 *
 * Generates release instructions for each of the processes' resoures and forces
 * it to execute those instructions.
 *
 * @param pcb The process chosen to be reset and release all of its resources.
 *
 */
void resolve_deadlock()
{

    printf("Function resolve_deadlock not implemented\n");
}

/**
 * @brief Deallocates the queues
 */
void free_manager(void)
{
#ifdef DEBUG_MNGR
    print_queue(readyq, "Ready");
    print_queue(waitingq, "Waiting");
    print_queue(terminatedq, "Terminated");
#endif

#ifdef DEBUG_MNGR
    printf("\nFreeing the queues...\n");
#endif
    dealloc_pcb_list(readyq.first);
    dealloc_pcb_list(waitingq.first);
    dealloc_pcb_list(terminatedq.first);
}

/**
 * @brief Retrieves the name of a process file or the codename "generator" from the list of arguments
 */
char *get_init_data(int num_args, char **argv)
{
    char *data_origin = "generate";
    if (num_args > 1)
        return argv[1];
    else
        return data_origin;
}

/**
 * @brief Retrieves the name of a process file or the codename "generator" from the list of arguments
 */
char *get_data(int num_args, char **argv)
{
    char *data_origin = "generate";
    if (num_args > 2)
        return argv[2];
    else
        return data_origin;
}

/**
 * @brief Retrieves the scheduler algorithm type from the list of arguments
 */
int get_algo(int num_args, char **argv)
{
    if (num_args > 3)
        return atoi(argv[3]);
    else
        return 1;
}

/**
 * @brief Retrieves the time quantum from the list of arguments
 */
int get_time_quantum(int num_args, char **argv)
{
    if (num_args > 4)
        return atoi(argv[4]);
    else
        return 1;
}

/**
 * @brief Print the arguments of the program
 */
void print_args(char *data1, char *data2, int sched, int tq)
{
    printf("Arguments: data1 = %s, data2 = %s, scheduler = %s,  time quantum = %d\n", data1, data2, (sched == 0) ? "priority" : "RR", tq);
}

/**
 * @brief Print the names of the global resources available in the system in linked list order
 */
void print_avail_resources(void)
{
    struct resource_t *resource;

    printf("Available:");
    for (resource = get_available_resources(); resource != NULL; resource = resource->next)
    {
        if (resource->available == YES)
        {
            printf(" %s", resource->name);
        }
    }
    printf(" ");
}

/**
 * @brief Print the names of the resources allocated to <code>process</code> in linked list order.
 */
void print_alloc_resources(pcb_t *proc)
{
    struct resource_t *resource;

    if (proc)
    {
        printf("Allocated to %s:", proc->process_in_mem->name);
        for (resource = proc->resources; resource != NULL; resource = resource->next)
        {
            printf(" %s", resource->name);
        }
        printf(" ");
    }
}

/**
 * @brief Print <code>msg</code> and the names of the processes in <code>queue</code> in linked list order.
 */
void print_queue(pcb_queue_t queue, char *msg)
{
    pcb_t *proc = queue.first;

    printf("%s:", msg);
    while (proc != NULL)
    {
        printf(" %s", proc->process_in_mem->name);
        proc = proc->next;
    }
    printf(" ");
}

/**
 * @brief Print <code>msg</code> and the names of the process currently running
 */
void print_running(pcb_t *proc, char *msg)
{
    printf("%s:", msg);
    if (proc != NULL)
    {
        printf(" %s", proc->process_in_mem->name);
    }
    printf(" ");
}

/**
 * @brief Print a linked list of instructions
 */
void print_instructions(instr_t *instr)
{
    instr_t *tmp_instr = instr;
    while (tmp_instr != NULL)
    {
        switch (tmp_instr->type)
        {
        case REQ_OP:
            printf("(req %s)\n", tmp_instr->resource_name);
            break;
        case REL_OP:
            printf("(rel %s)\n", tmp_instr->resource_name);
            break;
        case SEND_OP:
            printf("(send %s %s)\n", tmp_instr->resource_name, tmp_instr->msg);
            break;
        case RECV_OP:
            printf("(recv %s %s)\n", tmp_instr->resource_name, tmp_instr->msg);
            break;
        }
        tmp_instr = tmp_instr->next;
    }
}
