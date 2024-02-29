#ifndef PCB_H
#define PCB_H
#include <stdbool.h>

#define MAX_PAGES (10)
/*
 * Struct:  PCB 
 * --------------------
 * Fields:
 * priority - A boolean flag indicating if the process has priority status
 * currentPage - An integer storing the current page number the process is accessing
 * pid - An integer representing the process identifier
 * PC - Program Counter, an integer indicating the current execution point within the process
 * start - An integer marking the starting point of the process in memory
 * end - An integer marking the end point of the process in memory
 * job_length_score - An integer used for scheduling, representing the process's job length
 * pageCounter - An integer tracking the number of pages loaded for this process
 * currentLine - An integer indicating the current line being executed within the process
 * filename - A string storing the name of the file associated with the process
 * pageLoaded - An array of booleans indicating the loading status of each page
 * file - A pointer to the FILE structure associated with the process's program code
 */
typedef struct
{
    bool priority;
    int currentPage;
    int pid;
    int PC;
    int start;
    int end;
    int job_length_score;
    int pageCounter;
    int currentLine;
    char *filename;
    bool pageLoaded[MAX_PAGES];
    FILE* file;
}PCB;

int generatePID();
PCB * makePCB();
int getPageIdx(PCB* pcb);
#endif