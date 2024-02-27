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
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->start  = 0;
    newPCB->end = 0;
    newPCB->PC = newPCB->start;
    newPCB->job_length_score = 1+(newPCB->end)-(newPCB->start);
    newPCB->priority = false;
    newPCB->pageCounter = 0;
    
    for (int i = 0; i < MAX_PAGES; i++) {
        newPCB->pagetable[i] = -1;
        pcb->pageLoaded[i] = false;
    }

    return newPCB;
}

int getPageIdx(PCB* pcb) {
    for (int i = 9; i >= 0; i--) {
        if (pcb->pagetable[i] != -1) {
            return i + 1;
        }
    }
    return 0;
}