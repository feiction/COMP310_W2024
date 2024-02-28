#ifndef PCB_H
#define PCB_H
#include <stdbool.h>

#define MAX_PAGES (10)
/*
 * Struct:  PCB 
 * --------------------
 * pid: process (task) id
 * programCount: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 * pageTable: array to store page numbers
 * pageLoaded: array to track loaded pages
 * completed: boolean showing whether program is loaded completely
 * *fp: file that we need to read
 * *filename: name of the file for shellmemory
 */
typedef struct
{
    bool priority;
    int pid;
    int programCount;
    int start;
    int end;
    int job_length_score;
    int pageTable[MAX_PAGES];
    bool pageLoaded[MAX_PAGES];
    bool completed;
    FILE *fp;
    char *filename;
} PCB;

int generatePID();
PCB * makePCB();
int getPageIdx(PCB* pcb);
#endif