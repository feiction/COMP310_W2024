#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

struct frame_struct {
    char *fileID;
    char *line;
};

struct variable_struct {
    char *var;
    char *value;
};

void mem_init();
void mem_init_variable();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pStart, int* pEnd, char* fileID);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
#endif