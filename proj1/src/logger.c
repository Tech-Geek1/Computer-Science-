#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "logger.h"

/* Open the file for the current thread: in append mode */
FILE* open_logfile() {
    FILE* fptr = NULL;
    char* filename = NULL;

    if (!filename) {
        filename = malloc(64*sizeof(char));
        if (filename)
            sprintf(filename,"scheduler.log");
        else
            return NULL;
    }

    fptr = fopen(filename, "a");
    if (!fptr)
        return NULL; 

    free(filename);
    return fptr;
}

/* Close the logfile pointed to by fptr */
void close_logfile(FILE* fptr) {
    fclose(fptr);
}

/* Logging request resource */
void log_request_acquired(char* proc_name, char* resource_name) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s req %s: acquired\n", proc_name, resource_name);    
    printf("%s req %s: acquired\n", proc_name, resource_name);    
    fflush(fptr);
    close_logfile(fptr);
}

void log_request_waiting(char* proc_name, char* resource_name) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s req %s: waiting\n", proc_name, resource_name);    
    printf("%s req %s: waiting\n", proc_name, resource_name);    
    fflush(fptr);
    close_logfile(fptr);
}

void log_request_ready(char* proc_name) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s: ready\n", proc_name);    
    printf("%s: ready\n", proc_name);    
    fflush(fptr);
    close_logfile(fptr);
}

void log_release_released(char* proc_name, char* resource_name) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s rel %s: released\n", proc_name, resource_name);
    printf("%s rel %s: released\n", proc_name, resource_name);
    fflush(fptr);
    close_logfile(fptr);
}

void log_release_error(char* proc_name, char* resource_name) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s rel %s: error nothing to release\n", proc_name, resource_name);
    printf("%s rel %s: error nothing to release\n", proc_name, resource_name);
    fflush(fptr);
    close_logfile(fptr);
}

void log_terminated(char *proc_name) {
    FILE* fptr = open_logfile();
//    fprintf(fptr, "%s terminated\n", proc_name);
    printf("%s terminated\n", proc_name);
    fflush(fptr);
    close_logfile(fptr);
}

void log_send(char *proc_name, char* msg, char* mailbox) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s sending message%s to mailbox %s\n", proc_name, msg, mailbox);
    printf("%s sending message%s to mailbox %s\n", proc_name, msg, mailbox);
    fflush(fptr);
    close_logfile(fptr);
}

void log_recv(char *proc_name, char* msg, char* mailbox) {
    FILE* fptr = open_logfile();
    fprintf(fptr, "%s received message%s from mailbox %s\n", proc_name, msg, mailbox); 
    printf("%s received message%s from mailbox %s\n", proc_name, msg, mailbox); 
    fflush(fptr);
    close_logfile(fptr);
}

void log_deadlock_detected() {
    FILE* fptr = open_logfile();
    fprintf(fptr, "Deadlock detected:");
    printf("Deadlock detected:");
    fflush(fptr);
    close_logfile(fptr);
}

void log_blocked_procs() {
    FILE* fptr = open_logfile();
    fprintf(fptr, "No deadlock detected, but blocked process(es) found:");
    printf("No deadlock detected, but blocked process(es) found:");
    fflush(fptr);
    close_logfile(fptr);
}
