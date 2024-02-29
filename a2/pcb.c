#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(){
    PCB * pcb = malloc(sizeof(PCB));
    pcb->pid = generatePID();
    pcb->start  = 0;
    pcb->end = 0;
    pcb->PC = pcb->start;
    pcb->job_length_score = 1+(pcb->end)-(pcb->start);
    pcb->priority = false;
    pcb->currentPage = 1;
    pcb->currentLine = 0;
    pcb->pageCounter = 0;
    pcb->filename = NULL;
    pcb->file = NULL;
    
    for (int i = 0; i < MAX_PAGES; i++) {
        pcb->pagetable[i] = -1;
        pcb->pageLoaded[i] = false;
    }

    return pcb;
}

int getPageIdx(PCB* pcb) {
    for (int i = 9; i >= 0; i--) {
        if (pcb->pagetable[i] != -1) {
            return i + 1;
        }
    }
    return 0;
}