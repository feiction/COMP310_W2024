// Felicia Chen-She 261044333
// Christine Pan 260986437

#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include <dirent.h>
#include "pcb.h"
#include "shellmemory.h"

#if defined(FRAME_STORE_SIZE)
#else
const int FRAME_STORE_SIZE = 18;
#endif
#if defined(VAR_STORE_SIZE)
#else
const int VAR_STORE_SIZE = 10;
#endif

#define SHELL_MEM_LENGTH (FRAME_STORE_SIZE + VAR_STORE_SIZE)

struct memory_struct{
	char *var;
	char *value;
    int age;
    int accessed;
};

PagePolicy current_policy = LRU; // Default policy

void set_memory_policy(PagePolicy policy) {
    current_policy = policy;
}

struct memory_struct shellmemory[SHELL_MEM_LENGTH];

const int FRAME_SIZE = 3;
const int THRESHOLD = FRAME_STORE_SIZE;

int global_access_time = 0;     // initialize global access time

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

void mem_init() {
    int i;
    for (i = 0; i < SHELL_MEM_LENGTH; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
        shellmemory[i].accessed = 0;
        shellmemory[i].age = 0;
    }
}

// Part or resetmem()
// resets the variable memory back to none
void mem_init_variable(){
	for (int i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
    int i;
    for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }

    for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        if (strcmp(shellmemory[i].var, "none") == 0) {
            shellmemory[i].var = strdup(var_in);
            shellmemory[i].value = strdup(value_in);
            return;
        }
    }
}

// Get value based on input key
char *mem_get_value(char *var_in) {
    int i;
    for (i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        if (strcmp(shellmemory[i].var, var_in) == 0) {
            shellmemory[i].accessed = ++global_access_time;  // Update access time
            return strdup(shellmemory[i].value);
        }
    }
    return NULL;
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
        shellmemory[i].accessed = ++global_access_time;
        shellmemory[i].age = global_access_time;
	}
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


/* 
 * Finds the first available memory slot that can accommodate a frame of size FRAME_SIZE
 * Returns the start index of the available slot or -1 if no slot is found
 */
int find_available_slot() {
    bool slot_found = false;
    int start_index = 0;
    while (!slot_found && start_index < THRESHOLD) {
        if (strcmp(shellmemory[start_index].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 1].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 2].var, "none") == 0) {
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
 * Identifies the least recently used (LRU) memory slot based on access time
 * Returns the index of the LRU frame or -1 if all slots are available
 */
int find_lru_slot() {
    int lru_index = -1;
    int lru_time = 9999; // Initialize a high value

    for (int i = 0; i < THRESHOLD; i += FRAME_SIZE) {
        // Check the accessed time of the first unit in the current frame
        int frame_access_time = shellmemory[i].accessed;

        // Update the LRU frame if the current one has an older (smaller) access time
        if (frame_access_time < lru_time) {
            lru_time = frame_access_time;
            lru_index = i;
        }
    }
    return lru_index;
}

/* 
 * Identifies the first in memory slot based on age
 * Returns the index of the first in frame or -1 if all slots are available
 */
int find_fifo_slot() {
    int fifo_index = -1;
    int earliest_age = 9999; // Initialize a high value

    for (int i = 0; i < THRESHOLD; i += FRAME_SIZE) {
        // Check the age of the first unit in the current frame
        int frame_age = shellmemory[i].age;

        // Check each frame and find the one with the smallest age value
        if (frame_age < earliest_age) {
            earliest_age = frame_age;
            fifo_index = i;
        }
    }

    return fifo_index;
}

/* 
 * Identifies the appropriate memory slot based on policy
 * Returns the index of the first in frame or -1 if all slots are available
 */
int find_replacement_slot() {
    if (current_policy == LRU) {
        return find_lru_slot();
    } else { // FIFO
        return find_fifo_slot();
    }
}

/*
 * Loads 2 pages a file into memory and updating a PCB for the program
 * Allocates memory frames to the process, updates the PCB's page table, and reads the file
 * content into the shellmemory (framestore)
 *
 * Parameters:
 * FILE* fp - file pointer to the script or program being loaded
 * PCB* pcb - pointer to the PCB to be updated for the loaded program
 * char* filename - name of the file being loaded into memory
 *
 * Returns:
 * int - error code indicating the status of file loading operation.
 */
int load_file(FILE* fp, PCB* pcb, char* filename) {
    int error_code = 0;
    size_t frame_index = find_available_slot();

    if (frame_index == -1) {
        error_code = 21;    // no available memory slot
        return error_code;
    }

    // Initialize some variables
    pcb->start = frame_index;
    pcb->PC = pcb->start;
	pcb->filename = strdup(filename);
	pcb->file = fp;
    int lines_loaded = 0;
    bool load_next_page = true;

    // Read the file and load 2 pages its content into memory until the EOF or memory limit
    while (!feof(fp) && load_next_page && frame_index < THRESHOLD - 2) {
        size_t frame_start = frame_index;
        for (int i = 0; i < FRAME_SIZE && lines_loaded < (FRAME_SIZE * 2); i++) {
            if (feof(fp)) {
                pcb->end = frame_index - 1;
                break;
            }

            char *line = calloc(1, sizeof(char) * 100);
            if (fgets(line, sizeof(char) * 100, fp) == NULL) {
                continue;
            }

            // Store the line in memory
            shellmemory[frame_index].var = strdup(filename);
            shellmemory[frame_index].value = strndup(line, strlen(line));
            shellmemory[frame_index].accessed = ++global_access_time;
            shellmemory[frame_index].age = global_access_time;
            free(line);
            frame_index++;
            lines_loaded++;

            // Check if page completed
            if (lines_loaded % FRAME_SIZE == 0 || feof(fp)) {
                pcb->pagetable[pcb->pageCounter] = true; // mark page as loaded
                pcb->pageCounter++;

                // If two pages already loaded, stop loading more pages
                if (pcb->pageCounter == 2) {
                    load_next_page = false;
                }
            }
        }
        // Check for memory limit exceeded
        if (frame_index > THRESHOLD) {
            error_code = 21;
            break;
        }
    }

    pcb->end = frame_index - 1;

    return error_code;
}

/*
 * Loads a frame of data for a specific PCB from its associated file
 * It seeks to the appropriate position in the file based on the PCB's page table
 * and loads a frame of data into memory, updating the PCB's information
 *
 * Parameters:
 * PCB* pcb - pointer to the PCB to be updated for the loaded program
 *
 * Returns:
 * int - error code indicating the status of the frame loading operation
 * error code -1: 
 */
int load_frame(PCB* pcb) {
    char *line;
    int error_code = 0;
    FILE* fp;
	char* filename = pcb->filename;

	fp = fopen(filename, "r");

    // Skip over lines already loaded to the page table
	for (int i = 0; i < pcb->pageCounter*FRAME_SIZE; i++){
		line = calloc(1, sizeof(char) * 100);
		fgets(line, sizeof(char) * 100, fp);
        free(line);
        if (feof(fp)) {
            return -2;              // no more lines to be read
        }
		
	}

    if (feof(fp)) {
		return -2;                  // no more lines to be read
	}

    // Find available slot for loading the frame
    size_t frame_index = find_available_slot();
    if (frame_index == -1) {
        return -1;                  // no more space in the frame store
    }

    pcb->start = frame_index;  
    pcb->PC = frame_index;
	   
	// Load the frame into memory
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
        shellmemory[frame_index].accessed = ++global_access_time;
        shellmemory[frame_index].age = global_access_time;
		free(line);
		frame_index++;
	}

    // Update PCB
	pcb->pagetable[pcb->pageCounter] = true;
    pcb->pageCounter++;
	
    pcb->end = frame_index - 1;

    //printShellMemory();

    return error_code;
}

/*
 * Removes a frame from memory for a given PCB, used during a page fault handling
 * Identifies the LRU frame and clears its contents from memory,
 * freeing up space and updating the PCB's page table accordingly.
 *
 * Parameters:
 * PCB* pcb - pointer to the PCB from which a frame is to be removed
 *
 * Returns:
 * int - error code indicating the status of the frame removal operation
 */
int remove_frame(PCB* pcb) {
    char *line;
    int error_code = 0;
    FILE* fp;
    char* filename = pcb->filename;
    
    fp = fopen(filename, "r");
    
    if (fp == NULL) {
        return -1;
    }
    
    // Find the frame index of the first page table entry
    size_t frame_index = find_replacement_slot();
	pcb->start = frame_index;
	pcb->PC = pcb->start;
    int page_index = 0;
	int i;
	printf("%s\n", "Page fault! Victim page contents:");

    // Display and free the memory for the victim page content within the specified range
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (shellmemory[frame_index + i].var != NULL) {
            printf("%s", shellmemory[frame_index + i].value);
        }
    }
    
    mem_free_lines_between(frame_index, frame_index + FRAME_SIZE - 1);

    printf("%s\n", "End of victim page contents.");

	pcb->pagetable[page_index] = false;
	page_index++;
    
    fclose(fp);
    
    return error_code;
}

char *mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
    shellmemory[index].accessed = ++global_access_time;
	return shellmemory[index].value;
}

void free_shell_memory()
{
	for (int i = 0; i < SHELL_MEM_LENGTH; i++)
	{
		if (shellmemory[i].var != NULL && strcmp(shellmemory[i].var, "none") != 0)
		{
			free(shellmemory[i].var);
		}
		if (shellmemory[i].value != NULL && strcmp(shellmemory[i].value, "none") != 0)
		{
			free(shellmemory[i].value);
		}
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}