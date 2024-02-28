#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include <dirent.h>
#include "pcb.h"

#define SHELL_MEM_LENGTH 1000


struct memory_struct{
	char *var;
	char *value;
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH];

#if defined(FRAME_STORE_SIZE)
#else
const int FRAME_STORE_SIZE = 18;
#endif
#if defined(VAR_STORE_SIZE)
#else
const int VAR_STORE_SIZE = 10;
#endif

const int FRAME_SIZE = 3;
const int THRESHOLD = FRAME_STORE_SIZE;


// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions

void mem_init(){
	int i;
	for (i=0; i<1000; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

void mem_init_variable(){
	for (int i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	return;

}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}


void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH-count_empty, count_empty);
}
int find_available_slot() {
    bool slot_found = false;
    int start_index = 0;
    while (!slot_found && start_index < THRESHOLD-2) {
        if (strcmp(shellmemory[start_index].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 1].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 2].var, "none") == 0) {
            slot_found = true;
        } else {
            start_index+=FRAME_SIZE;
        }
    }

    if (slot_found) {
        return start_index;
    } else {
        return -1;
    }
}

int find_unavailable_slot() {
    bool slot_found = false;
    int start_index = 0;
    while (!slot_found && start_index < THRESHOLD - 2) {
        if (strcmp(shellmemory[start_index].var, "none") != 0){
            slot_found = true;
        } else {
            start_index += FRAME_SIZE;
        }
    }

    if (slot_found) {
        return start_index;
    } else {
        return -1;
    }
}

/*
 * Function:  addFileToMem 
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here
 *	pEnd: This function will store the last line of the loaded file 
 			in shell memory in here
 *  fileID: Input that need to provide when calling the function, 
 			stores the ID of the file
 * 
 * returns: error code, 21: no space left
 */
int load_file(FILE* fp, PCB* pcb, char* filename) {
    char *line;
    size_t i = 0;
    int error_code = 0;
    bool flag = true;
    size_t frame_index = find_available_slot();
    if (frame_index == -1) {
        error_code = 21;
        return error_code;
    }
    size_t page_index = 0;
    pcb->start = frame_index;
    pcb->PC = pcb->start;
	pcb->filename = strdup(filename);
	pcb->file = fp;

    int lines_loaded = 0;
    bool load_next_page = true;

    while (!feof(fp) && load_next_page && frame_index <THRESHOLD-2) {
        size_t frame_start = frame_index;
        for (int i = 0; i < FRAME_SIZE && lines_loaded < 6; i++) {
            if (feof(fp)) {
                pcb->end = frame_index - 1;
                break;
            }

            line = calloc(1, sizeof(char) * 100);
            if (fgets(line, sizeof(char) * 100, fp) == NULL) {
                continue;
            }

            shellmemory[frame_index].var = strdup(filename);
            shellmemory[frame_index].value = strndup(line, strlen(line));
            free(line);
            frame_index++;
            lines_loaded++;

            // Check if page completed
            if (lines_loaded % 3 == 0 || feof(fp)) {
                pcb->pagetable[page_index] = (frame_start) / 3;
                pcb->pageLoaded[page_index] = true; // mark page as loaded
                page_index++;
                pcb->pageCounter++;

                // If two pages loaded or the file is small, stop loading more pages.
                if (page_index == 2 || feof(fp)) {
                    load_next_page = false;
                }

            }
        }
       
        if (frame_index > THRESHOLD) {
            error_code = 21;
            break;
        }
    }

    pcb->end = frame_index - 1;
    //printShellMemory();
    return error_code;
}

int load_frame(PCB* pcb) {
    char *line;
    int error_code = 0;
    FILE* fp;
	char* filename = pcb->filename;
   // fp = pcb->file;
	fp = fopen(filename, "r");
	
    
   
	int i;
	for (i = 0; i < MAX_PAGES; i++) {
        if (pcb->pagetable[i] == -1) {
            break;
        }
    }
    size_t page_index = i;

	for (int i = 0; i< pcb->pageCounter*3; i++){
		line = calloc(1, sizeof(char) * 100);
		fgets(line, sizeof(char) * 100, fp);
        if (feof(fp)) {
            return -2;
        }
		free(line);
	}

    if (feof(fp)) {
		return -2;
	}
    size_t frame_index = find_available_slot();
    if (frame_index == -1) {
        error_code = -1;
        return error_code;
    }


    pcb->start = frame_index;  
    pcb->PC = frame_index;
	
	while (!feof(fp)){
       
		frame_index = find_available_slot();
		size_t frame_start = frame_index;
   
		if (frame_index == -1) {
			return -1;
		}
		for (int i = 0; i < FRAME_SIZE; i++) {
			if (feof(fp)) {
				pcb->end = frame_index - 1;
				break;
			}
			line = calloc(1, sizeof(char) * 100);
            if (fgets(line, sizeof(char) * 100, fp) == NULL) {
                continue;
            }
			shellmemory[frame_index].var = strdup(filename);
			shellmemory[frame_index].value = strndup(line, strlen(line));
			free(line);
			frame_index++;

		}

		pcb->pagetable[page_index] = (frame_start) / 3;
		pcb->pageLoaded[page_index] = true;
		page_index++;
        pcb->pageCounter++;
	
	}
	pcb->pageFault = false;
    pcb->end = frame_index - 1;

    //printShellMemory();

    return error_code;
}

int remove_frame(PCB* pcb) {
    char *line;
    int error_code = 0;
    FILE* fp;
    char* filename = pcb->filename;
    
    // Open the file
    fp = fopen(filename, "r");
    
    // Check if the file opened successfully
    if (fp == NULL) {
        printf("Error opening file '%s'.\n", filename);
        return -1;
    }
    
    // Find the frame index of the first page table entry
    size_t frame_index = find_unavailable_slot();
	pcb->start = frame_index;
	pcb->PC = pcb->start;
    int page_index = 0;
	int i;
	printf("%s\n", "Page fault! Victim page contents:");
    // Loop through each page table entry
    for(i = 0; i < FRAME_SIZE; i++){
        // Check if the page is loaded
        if (shellmemory[frame_index].var != NULL) {
			printf("%s", shellmemory[frame_index].value);
            free(shellmemory[frame_index].var);
            free(shellmemory[frame_index].value);
            
            // Mark the frame as available by resetting its entry in shellmemory
            shellmemory[frame_index].var = "none";
            shellmemory[frame_index].value = "none";
            frame_index++;
            // Update page table and pageLoaded arrays
        }
        
        // Move to the next frame index
    }
    printf("%s\n", "End of victim page contents.");
	
	//load_frame(pcb);
    
	pcb->pagetable[page_index] = -1;
	pcb->pageLoaded[page_index] = false;
	page_index++;
    
    // Close the file
    fclose(fp);
     
    // Reset page fault flag
    pcb->pageFault = false;
    
    // Print the updated shell memory
    //printShellMemory();
	/*for (int i = 0; i < MAX_PAGES; i++) {
        printf("Page %d: %d, %d\n", i, pcb->pagetable[i], pcb->pageFault);
    }*/
    
    return error_code;
}

char * mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}


void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
		if(shellmemory[i].var != NULL){
			free(shellmemory[i].var);
		}	
		if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}