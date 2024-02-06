// Felicia Chen 261044333
// Christine Pan 260986437
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <sys/stat.h>
#include "shellmemory.h"
#include "shell.h"
#include <stdbool.h>

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

// For wc command only
int badcommandWC(){
	printf("%s\n", "Bad command: my_wc");
	return 7;
}

// For if commands
int badcommandEmptyif(){
	printf("%s\n", "Empty if clause");
	return 6;
}

int badcommandIf(){
	printf("%s\n", "Bad command: if");
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
int my_wc(char* filename);

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;

	if ( args_size < 1 ) {
		return badcommand();
	} else if (args_size > MAX_ARGS_SIZE) {
		if ((strcmp(command_args[0], "set")!=0) && (strcmp(command_args[0], "if")!=0))
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
		char value[800];						// up to 5 tokens of 100 characters, 800 to be safe
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
		if (args_size != 2) return badcommandCD();
		return my_cd(command_args[1]);

	} else if (strcmp(command_args[0], "my_cat")==0) {
		if (args_size != 2) return badcommandCat();
		return my_cat(command_args[1]);

	} else if (strcmp(command_args[0], "if") == 0) {
        char* identifier1 = command_args[1];
        char* op = command_args[2];
        char* identifier2 = command_args[3];
        char* then = command_args[4];
		bool condition = false;
		int else_index = -1;

		if (strcmp(command_args[5], "fi") == 0) return badcommandEmptyif();
		if (strcmp(op, "==") != 0 && strcmp(op, "!=") != 0) return badcommandIf();

		// Get values of identifiers if they're variables
		if (identifier1[0] == '$') {
			char *value = mem_get_value(++identifier1);
			if (value == NULL) return badcommandIf();
			identifier1 = value;
		}
		if (identifier2[0] == '$') {
			char *value = mem_get_value(++identifier2);
			if (value == NULL) return badcommandIf();
			identifier2 = value;
		}

		condition = (strcmp(identifier1, identifier2) == 0 && strcmp(op, "==") == 0) ||
                        (strcmp(identifier1, identifier2) != 0 && strcmp(op, "!=") == 0);
		
		char command[800];											// up to 5 tokens of 100 characters, 800 to be safe

		// Find else index
		for (i = 5; i < args_size; i++) {
			if (strcmp(command_args[i], "else")==0) {
				else_index = i;
				break;
			}
		}
		if (else_index == -1) return badcommandIf();				// no else found in if command

		// Execute appropriate command
		if (condition) {
			strcpy(command, command_args[5]);						// store string in command
			for (i = 6; i < else_index; i++) {
				strcat(command, " ");								// insert space
				strcat(command, command_args[i]);					// add string to command
			}
			parseInput(command);
		} else {
			strcpy(command, command_args[else_index + 1]);
			for (i = (else_index + 2); i < (args_size - 1); i++) {
				strcat(command, " ");								// insert space
				strcat(command, command_args[i]);					// add string to command
			}
			parseInput(command);
		}
    } else if (strcmp(command_args[0], "my_wc") == 0) {
		if (args_size != 2) return badcommand();
		return my_wc(command_args[1]);
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
	fflush(stdout);
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
	return system("ls");		// returns 0 on success
}

int my_mkdir(char *dirname){
	return mkdir(dirname, 0700);	// Create the directory with 0700 permissions
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
		
    	fclose(file);
    	return 0; 
	} 
	else return badcommandCat();
}

int my_wc(char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) return badcommandWC();
    

    int lines = 0, words = 0, characters = 0;
    int character;
    int inWord = 0;

    while ((character = getc(file)) != EOF) {
        characters++;

        // Check for newline character
        if (character == '\n') {
            lines++;
        }

        // Check for whitespace to track words
        if (character == ' ' || character == '\t' || character == '\n') {
            inWord = 0;
        } else if (inWord == 0) {
            inWord = 1;
            words++;
        }
    }

    fclose(file);

    printf("Lines: %d\nWords: %d\nCharacters: %d\n", lines, words, characters);

    return 0;
}
