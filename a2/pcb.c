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
PCB* makePCB(int start, int end){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start  = start;
    newPCB->end = end;
    newPCB->job_length_score = 1+end-start;
    newPCB->priority = false;

    /*
    for (int i = 0; i < MAX_PAGES / 3; i++) {
        newPCB->pagetable[i] = -1;
    }*/

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