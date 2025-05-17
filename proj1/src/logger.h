/**
  * @file logger.h
  * @description A definition of functions offered by the logger
  */

#ifndef LOGGER_H
#define LOGGER_H

/* Functions */
void log_request_acquired(char* proc_name, char* resource_name);
void log_request_waiting(char* proc_name, char* resource_name);
void log_request_ready(char* proc_name);
void log_release_released(char* proc_name, char* resource_name);
void log_release_error(char* proc_name, char* resource_name);
void log_terminated(char *proc_name);
void log_send(char *proc_name, char* msg, char* mailbox);
void log_recv(char *proc_name, char* msg, char* mailbox);
void log_deadlock_detected();
void log_blocked_procs();

#endif
