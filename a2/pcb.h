#ifndef PCB_H
#define PCB_H
#include <stdbool.h>

#define MAX_PAGES (10)
/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */
typedef struct
{
    bool priority;
    int pid;
    int PC;
    int start;
    int end;
    int job_length_score;
    int pageCounter;
    int pagetable[MAX_PAGES];
}PCB;

int generatePID();
PCB * makePCB();
int getPageIdx(PCB* pcb);
#endif