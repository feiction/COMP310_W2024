// Felicia Chen-She 261044333
// Christine Pan 260986437

#ifndef KERNEL
#define KERNEL
#include "pcb.h"
int process_initialize(char *filename);
int schedule_by_policy(char* policy); //, bool mt);
void ready_queue_destroy();
#endif