#ifndef SHELL_H
#define SHELL_H
int parseInput(char *ui);
int copyScript(char *filename);
int count_files_backing();
char* get_backing_store_path();
#endif