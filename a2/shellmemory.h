#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
#include "pcb.h"
void mem_init();
void mem_init_variable();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int find_available_slot();
int load_file(FILE* fp, PCB* pcb, char* fileID);
int load_frame(PCB* pcb);
int remove_frame(PCB* pcb);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
#endif