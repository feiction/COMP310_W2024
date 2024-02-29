#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <sys/stat.h>
#include "interpreter.h"
#include "shellmemory.h"
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include <libgen.h>
#include <dirent.h>

int MAX_USER_INPUT = 1000;
int parseInput(char ui[]);
int my_mkdir(char *dirname);
int main(int argc, char *argv[]) {

    // Create or remove files in backing store when shell is created
    system("rm -rf ./backing_store");
    mkdir("./backing_store/", 0700);
    printf("%s\n", "Shell v2.0\n");

    // Print the frame store size and variable store size
    printf("Frame Store Size = %d; Variable Store Size = %d\n", FRAME_STORE_SIZE, VAR_STORE_SIZE);

    char prompt = '$';  				// Shell prompt
	char userInput[MAX_USER_INPUT];		// user's input stored here
	int errorCode = 0;					// zero means no error, default

	//init user input
	for (int i=0; i<MAX_USER_INPUT; i++)
		userInput[i] = '\0';
	
	//init shell memory
	mem_init();

	while(1) {						
        if (isatty(fileno(stdin))) printf("%c ",prompt);

		char *str = fgets(userInput, MAX_USER_INPUT-1, stdin);
        if (feof(stdin)){
            freopen("/dev/tty", "r", stdin);
        }

		if(strlen(userInput) > 0) {
            errorCode = parseInput(userInput);
            if (errorCode == -1) exit(99);	// ignore all other errors
            memset(userInput, 0, sizeof(userInput));
		}
	}

	return 0;

}

int parseInput(char *ui) {
    char tmp[200];
    char *words[100];
	memset(words, 0, sizeof(words));
	
    int a = 0;
    int b;                            
    int w=0; // wordID    
    int errorCode;
    for(a=0; ui[a]==' ' && a<1000; a++);        // skip white spaces
    
    while(a<1000 && a<strlen(ui) && ui[a] != '\n' && ui[a] != '\0') {
        while(ui[a]==' ') a++;
        if(ui[a] == '\0') break;
        for(b=0; ui[a]!=';' && ui[a]!='\0' && ui[a]!='\n' && ui[a]!=' ' && a<1000; a++, b++) tmp[b] = ui[a];
        tmp[b] = '\0';
        if(strlen(tmp)==0) continue;
        words[w] = strdup(tmp);
        if(ui[a]==';'){
            w++;
            errorCode = interpreter(words, w);
            if(errorCode == -1) return errorCode;
            a++;
            w = 0;
            for(; ui[a]==' ' && a<1000; a++);        // skip white spaces
            continue;
        }
        w++;
        a++; 
    }
    errorCode = interpreter(words, w);
    return errorCode;
}

/*
 * Counts the number of files executed
 *
 * Returns:
 * int - number of files
 */
int count_files_backing() {
    // Have to include dirent.h to access a specific directory
    struct dirent *directory_pointer;

    // Initialize count
    int count = 0;
    
    // Open directory and check 
    DIR *directory = opendir("backing_store");

    if (directory == NULL) {
        printf("Error opening current directory");
        return -1;
    }

    // Increment count
    while ((directory_pointer = readdir(directory)) != NULL) {
        count ++;
    }

    closedir(directory);

    // Calibrate count
    return count - 2;
}

/*
 * Copies the script to the backing_store directory
 *
 * Parameters:
 * char* filenmae - name of the file being loaded into memory
 * Returns:
 * int - error code
 */
int copyScript(char *filename) {
    FILE *scriptFile, *backingStoreFile;
    char ch;

    // Open original file
    scriptFile = fopen(filename, "r");
    if (scriptFile == NULL) {
        perror("Error opening script file");
        return 1;
    }

    // Naming
    const char *directoryName = "backing_store/";
    const char *scriptBaseName = basename(strdup(filename));
    int count = count_files_backing();

    char newFilename[256];
    snprintf(newFilename, sizeof(newFilename), "%sprog%d", directoryName, count);

    // Write character by character to the backing store
    backingStoreFile = fopen(newFilename, "w");
    
    if (backingStoreFile == NULL) {
        perror("Error creating new file in the backing store");
        fclose(scriptFile);
        return 1;
    }
    while ((ch = fgetc(scriptFile)) != EOF) {
        fputc(ch, backingStoreFile);
    }

    // Close files
    fclose(scriptFile);
    fclose(backingStoreFile);

    return 0;
}