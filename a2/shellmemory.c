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

//Helper functions for load_file

int find_available_slot() {
    bool slot_found = false;
    int start_index = 0;
    while (!slot_found && start_index < THRESHOLD - 2) {
        if (strcmp(shellmemory[start_index].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 1].var, "none") == 0 &&
            strcmp(shellmemory[start_index + 2].var, "none") == 0) {
            slot_found = true;
        } else {
            start_index+=FRAME_SIZE;
        }
    }
    if (slot_found)
        return start_index;
    return -1;
}

int find_lru_frame(PCB* pcb) {
    int lru_frame = -1;
    int lru_value = 0;
    
    // Find least recently used frame by going through lastAccessed array
    for (int i = pcb->start; i <= pcb->end; i++) {
        if (shellmemory[i].var != "none" && pcb->lastAccessed[i] < lru_value) {
            lru_value = pcb->lastAccessed[i];
            lru_frame = i;
        }
    }
    return lru_frame;
}

bool fetch_page(PCB *pcb) {
    int available_slot = find_available_slot();
    
    // If no available slot, find LRU page and evict it
    if (available_slot == -1) {
        int lru_frame_index = find_lru_frame(pcb);
        
        // LRU page found
        if (lru_frame_index != -1) {
            int page_number = lru_frame_index / FRAME_SIZE;
            
            // Print victim page contents
            printf("Page fault! Victim page contents:\n");
            for (int i = 0; i < FRAME_SIZE; i++) {
                int frame_index = (page_number * FRAME_SIZE) + i;
                if (shellmemory[frame_index].var != NULL) {
                    printf("%s\n", shellmemory[frame_index].value);
                }
            }
            printf("End of victim page contents.\n");

            bool replaced = replace_page_contents(pcb, lru_frame_index);
            return replaced;
        } else {
            // Failed to find a page to evict
            return false;
        }
    } else {
        // Load page into the available slot directly without eviction
        bool loaded = load_page_to_memory(pcb, available_slot);
        return loaded;
    }
}


int load_file(FILE* fp, PCB* pcb, char* filename) {
    char *line;
    int error_code = 0;
    size_t frame_index = find_available_slot();

     if (frame_index == -1) {
        error_code = 21;
        return error_code;
    }

    pcb->start = frame_index;
    pcb->programCount = pcb->start;
    size_t page_index = 0;
    int lines_loaded = 0;
    bool load_next_page = true;

	// Load pages from the file into memory
    while (!feof(fp) && load_next_page) {

        // Check if 2 pages are already loaded in memory
        if (page_index == 2) {
            pcb->pageFault = true;
            load_next_page = false;
        }

        // Proceed the loading
        size_t frame_start = frame_index;
        for (int i = 0; i < FRAME_SIZE && lines_loaded < 6; i++) {
            if (feof(fp)) {
                pcb->end = frame_index - 1;
                break;
            }

            line = calloc(1, THRESHOLD);
            if (fgets(line, THRESHOLD, fp) == NULL)
                continue;

            shellmemory[frame_index].var = strdup(filename);
            shellmemory[frame_index].value = strndup(line, strlen(line));
            free(line);
            frame_index++;
            lines_loaded++;

            // Check if page completed
            if (lines_loaded % 3 == 0 || feof(fp)) {
                pcb->pageTable[page_index] = (frame_start) / FRAME_SIZE;
                pcb->pageLoaded[page_index] = true; 		// mark page as loaded
				pcb->lastAccessed[page_index]++;            // update last accessed value
                page_index++;
            }
        }

        if (frame_index >= SHELL_MEM_LENGTH) {
            error_code = 21;
            break;
        }
    }

    pcb->end = frame_index - 1;

/* 	// Print PCB information for debugging
    printf("Program loaded into memory:\n");
    printf("Start: %d, End: %d\n", pcb->start, pcb->end);
    for (int i = 0; i < 2; i++) {
        printf("Page %d: Frame %d, Loaded: %s\n", i, pcb->pageTable[i], pcb->pageLoaded[i] ? "Yes" : "No");
    } */

    //printShellMemory();    // debug line
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