#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include <dirent.h>
#include <limits.h>
#include "pcb.h"

#define SHELL_MEM_LENGTH 1000


struct memory_struct{
	char *var;
	char *value;
    int access;
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
const int THRESHOLD = FRAME_STORE_SIZE * FRAME_SIZE;


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
        shellmemory[i].access = 0;
    }
}

void mem_init_variable(){
	for (int i = THRESHOLD; i < SHELL_MEM_LENGTH; i++) {
        shellmemory[i].var = "none";
        shellmemory[i].value = "none";
        shellmemory[i].access++;
    }
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
            shellmemory[i].access++;
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
            shellmemory[i].access++;
			return;
		} 
	}

	return;

}

void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
        shellmemory[i].access = 0;
        if (shellmemory[i].var != NULL){
            free(shellmemory[i].var);
        }
        if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=THRESHOLD; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
            shellmemory[i].access++;
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}

void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
            shellmemory[i].access++;
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
    int available_slot = 0;
    // Go through frame store to check for available space
    while (!slot_found && available_slot < THRESHOLD - 2) {
        if (strcmp(shellmemory[available_slot].var, "none") == 0 &&
            strcmp(shellmemory[available_slot + 1].var, "none") == 0 &&
            strcmp(shellmemory[available_slot + 2].var, "none") == 0) {
            slot_found = true;
        } else {
            available_slot += FRAME_SIZE;
        }
    }
    if (slot_found)
        return available_slot;
    return -1;
}

int load_file(FILE* fp, PCB* pcb, char* filename) {
    char *line;
    int error_code = 0;
    pcb->fp = fp;
    pcb->filename = filename;
    size_t available_slot = find_available_slot();

     if (available_slot == -1) {
        error_code = 21;
        return error_code;
    }

    pcb->start = available_slot;
    pcb->programCount = pcb->start;
    size_t page_index = 0;
    int lines_loaded = 0;

    // Load pages from the file into memory
    pcb->completed = true;               // mark as true, toggle only if feof not reached after loading 2 pages
    while (!feof(fp)) {

        // Check if 2 pages are already loaded in memory
        if (page_index == 2) {
            pcb->completed = false;     // eof not reached, program is not completed
            break;
        }

        // Proceed the loading
        size_t frame_start = available_slot;
        for (int i = 0; i < FRAME_SIZE && lines_loaded < 6; i++) {
            if (feof(fp)) {
                pcb->end = available_slot - 1;
                break;
            }

            line = calloc(1, THRESHOLD);
            if (fgets(line, THRESHOLD, fp) == NULL)
                continue;

            shellmemory[available_slot].var = strdup(filename);
            shellmemory[available_slot].value = strndup(line, strlen(line));
            shellmemory[available_slot].access++;
            free(line);
            available_slot++;
            lines_loaded++;

            // Check if page completed
            if (lines_loaded % 3 == 0 || feof(fp)) {
                pcb->pageTable[page_index] = (frame_start) / FRAME_SIZE;
                pcb->pageLoaded[page_index] = true; 		// mark page as loaded
                page_index++;
            }
        }

        if (available_slot >= SHELL_MEM_LENGTH) {
            error_code = 21;
            break;
        }
    }

    pcb->end = available_slot - 1;

/* 	// Print PCB information for debugging
    printf("Program loaded into memory:\n");
    printf("Start: %d, End: %d\n", pcb->start, pcb->end);
    for (int i = 0; i < 2; i++) {
        printf("Page %d: Frame %d, Loaded: %s\n", i, pcb->pageTable[i], pcb->pageLoaded[i] ? "Yes" : "No");
    } */

    //printShellMemory();    // debug line
    return error_code;
}

int find_lru_frame() {
    int min_access_sum = INT_MAX;  // initialize with max possible integer value
    int lru_frame_index = -1;      // no frame found
    int current_sum;

    // Check for the frame with lowest sum of access times
    for (int start_index = 0; start_index <= THRESHOLD - FRAME_SIZE; start_index += FRAME_SIZE) {
        current_sum = 0;
        
        // Sum the access times for current frame
        for (int i = 0; i < FRAME_SIZE; i++) {
            current_sum += shellmemory[start_index + i].access;
        }

        // Check if current frame has lowest sum of access times
        if (current_sum < min_access_sum) {
            min_access_sum = current_sum;
            lru_frame_index = start_index;
        }
    }

    return lru_frame_index;
}

bool fetch_page(PCB *pcb) {
    int available_slot = find_available_slot();
    int page_index = 0;
    bool pageLoaded = false;

    // Find the first unloaded page
    while (page_index < MAX_PAGES && pcb->pageLoaded[page_index]) {
        page_index++;
    }

    // Evict when no available slot
    if (available_slot == -1) { 
        available_slot = find_lru_frame();
        
        // Evict the contents of the LRU frame
        printf("Page fault! Victim page contents:\n");
        for (int i = 0; i < FRAME_SIZE; i++) {
            printf("%s\n", shellmemory[available_slot + i].value);
        }
        printf("End of victim page contents.\n");
        mem_free_lines_between(available_slot, available_slot + FRAME_SIZE - 1);
    }

    // Fetch 3 more lines from the file and store them in the framestore
    FILE *fp = pcb->fp;
    fseek(fp, page_index * FRAME_SIZE * sizeof(char) * THRESHOLD, SEEK_SET);

    char *line = calloc(1, THRESHOLD);
    int lines_loaded = 0;
    while (lines_loaded < FRAME_SIZE && fgets(line, THRESHOLD, fp) != NULL) {
        shellmemory[available_slot + lines_loaded].var = strdup(pcb->filename);
        shellmemory[available_slot + lines_loaded].value = strndup(line, strlen(line));
        shellmemory[available_slot + lines_loaded].access++;
        lines_loaded++;
    }
    free(line);

    // If less than FRAME_SIZE lines were loaded or EOF was reached, mark as completed
    if (lines_loaded < FRAME_SIZE || feof(fp)) {
        pcb->completed = true;
    }

    // Update PCB with the new page information
    pcb->pageTable[page_index] = available_slot / FRAME_SIZE;
    pcb->pageLoaded[page_index] = true;
    pageLoaded = true;

    return pageLoaded;
}

char * mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}