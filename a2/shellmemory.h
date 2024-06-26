// Felicia Chen-She 261044333
// Christine Pan 260986437

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
void free_shell_memory();
typedef enum { FIFO, LRU } PagePolicy;
extern PagePolicy current_policy;
void set_memory_policy(PagePolicy policy);
#endif