#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"
#define BACKING_STORE_DIR "backing_store/"
bool active = false;
bool debug = false;
bool in_background = false;

int process_initialize(char *filename){
    FILE* fp;
    FILE* fp2;
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    
    fp = fopen(filename, "rt");
    if(fp == NULL){
		return FILE_DOES_NOT_EXIST;
    }
    int count = count_files_backing();
    int copy_to_backing = copyScript(filename);
    if(copy_to_backing != 0){
        return FILE_ERROR;
    }
    fclose(fp);

    char backingstore_filename[strlen(filename) + strlen(BACKING_STORE_DIR) + 1];
    strcpy(backingstore_filename, BACKING_STORE_DIR);
    char newFilename[256];
    snprintf(newFilename, sizeof(newFilename), "prog%d", count);
    strcat(backingstore_filename, newFilename);
    fp2 = fopen(backingstore_filename, "rt");
    PCB* newPCB = makePCB();
    int error_code = load_file(fp2, newPCB, backingstore_filename);
    for (int i = 0; i < MAX_PAGES; i++) {
        printf("Page %d: %d\n", i, newPCB->pagetable[i]);
    }

    if(error_code != 0){
        fclose(fp2);
        return FILE_ERROR;
    }
   
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_tail(node);

    fclose(fp2);
    return 0;
}

bool execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    for(int i=0; i<quanta; i++){
<<<<<<< Updated upstream
        bool page_fault = pcb->currentPage > pcb->pageCounter;
       // printf("%s, %d, %d\n", pcb->filename, pcb->currentPage, pcb->pageCounter);
        if (page_fault) {
            //printf("here");
            int errorcode = load_frame(pcb);
            if (errorcode == -2){
                //printf("jere\n");
                in_background = false;
                terminate_process(node);
                return true;
            }
            while(errorcode == -1) {
                //printf("pcb file: %s\n", pcb->filename);
                remove_frame(pcb);
                    
                errorcode = load_frame(pcb);
            }
            return false;
        }
=======
>>>>>>> Stashed changes
        line = mem_get_value_at_line(pcb->PC++);
        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        if(pcb->PC>pcb->end){
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        if(strcmp(line, "none")!=0) {
            parseInput(line);
        }
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }   
    }
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg){
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        if(is_ready_empty()){
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
<<<<<<< Updated upstream
        if(execute_process(cur, quanta)) {
            
        }
        else{
            if (cur->pcb->pageFault) {
                //printf("hello");
            }
=======
        if(!execute_process(cur, quanta)) {
>>>>>>> Stashed changes
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char* policy){ //, bool mt){
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return SCHEDULING_ERROR;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(strcmp("FCFS",policy)==0){
        scheduler_FCFS();
    }else if(strcmp("SJF",policy)==0){
        scheduler_SJF();
    }else if(strcmp("RR",policy)==0){
        arg[0] = 2;
        scheduler_RR((void *) arg);
    }else if(strcmp("AGING",policy)==0){
        scheduler_AGING();
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        scheduler_RR((void *) arg);
    }
    return 0;
}

