# Process Management



This project is a process simulation implemented in C. It simulates the execution of processes with different scheduling algorithms such as FCFS (First-Come, First-Served) and Priority with Preemption.



## Overview


The simulation consists of processes loaded from provided files. These processes are then scheduled for execution using one of the available scheduling algorithms. The simulation manages resources requested and released by the processes and handles process states such as READY, WAITING, and TERMINATED.



## Files


1. `logger.h`: Header file for logging functions used in the simulation.
2. `manager.h`: Header file containing function declarations for process management.
3. `manager.c`: Source file containing implementations for process management functions.
4. `proc_structs.h`: Header file defining the data structures used in the simulation.
5. `proc_syntax.h`: Header file defining the syntax and types used for instructions.



## Usage
Go to root, 
type make, 
Run the run.sh file or

e.g) `./schedule_processes data/process1.list data/process2.list 0 2 `
