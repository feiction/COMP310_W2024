#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/stat.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 3;

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}

// For set command only
int badcommandSet(){
	printf("%s\n", "Bad command: set");
	return 4;
}

// For change directory command only
int badcommandCD(){
	printf("%s\n", "Bad command: my_cd");
	return 5;
}

// For cat command only
int badcommandCat(){
	printf("%s\n", "Bad command: my_cat");
	return 6;
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int echo(char *token);
int my_ls();
int my_mkdir(char* dirname);
int my_touch(char* filename);
int my_cd(char* dirname);
int my_cat(char* newfile);

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;

	if ( args_size < 1 || (args_size > MAX_ARGS_SIZE && strcmp(command_args[0], "set")!=0)){
		return badcommand();
	}

	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		//set
		if (args_size < 3 || args_size > 7) return badcommandSet();
		char value[800];						// up to 7 tokens of 100 characters, 800 to be safe
		strcpy(value, command_args[2]);			// store string in value
		for (i = 3; i < args_size; i++) {
			strcat(value, " ");					// insert space
			strcat(value, command_args[i]);		// add string to value
		}
		return set(command_args[1], value);		// return set(variable, value)

	} else if (strcmp(command_args[0], "print")==0) {
		//print
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "run")==0) {
		if (args_size != 2) return badcommand();
		return run(command_args[1]);

	} else if (strcmp(command_args[0], "echo")==0){
		if (args_size > 2) return badcommand();
		return echo(command_args[1]);

	} else if (strcmp(command_args[0], "my_ls")==0) {
		if (args_size > 1) return badcommand();
		return my_ls();

	} else if (strcmp(command_args[0], "my_mkdir")==0) {
		if (args_size != 2) return badcommand();
		return my_mkdir(command_args[1]);

	} else if (strcmp(command_args[0], "my_touch")==0) {
		if (args_size != 2) return badcommand();
		return my_touch(command_args[1]);

	} else if (strcmp(command_args[0], "my_cd")==0) {
		if (args_size != 2) return badcommand();
		return my_cd(command_args[1]);

	} else if (strcmp(command_args[0], "my_cat")==0) {
		if (args_size != 2) return badcommand();
		return my_cat(command_args[1]);

	} else return badcommand();
}

int help(){

	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	exit(0);
}

int set(char* var, char* value){
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);

	mem_set_value(var, value);

	return 0;

}

int print(char* var){
	char *value = mem_get_value(var);
	if(value == NULL) value = "\n";
	printf("%s\n", value); 
	return 0;
}

int run(char* script){
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script,"rt");  // the program is in a file

	if(p == NULL){
		return badcommandFileDoesNotExist();
	}

	fgets(line,999,p);
	while(1){
		errCode = parseInput(line);	// which calls interpreter()
		memset(line, 0, sizeof(line));

		if(feof(p)){
			break;
		}
		fgets(line,999,p);
	}

    fclose(p);

	return errCode;
}

int echo(char* var){
	if (var[0] == '$')
		print(++var);
	else
		printf("%s\n", var); 
	return 0; 
}

int my_ls(){
	return system("ls");
}

int my_mkdir(char *dirname){
	// Create the directory with 0700 permissions
	return mkdir(dirname, 0700);
}

int my_touch(char* filename){
	FILE *newfile;
	newfile = fopen(filename, "w");
    fclose(newfile);
	return 0;
}

int my_cd(char* dirname){
	if (chdir(dirname) == 0) return 0;
	else return badcommandCD();
}

int my_cat(char* filename){
	FILE *file = fopen(filename, "r");
	if (file != NULL) {
		int character;
		while ((character = getc(file)) != EOF) {
			putchar(character);
		}
		putchar('\n'); // do we need a new line? i added for visual
    	fclose(file);
    	return 0; 
	} 
	else {
		badcommandCat();
	}
}