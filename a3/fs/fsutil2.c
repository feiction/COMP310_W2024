#include "fsutil2.h"
#include "../interpreter.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SECTOR_SIZE 512 // Assuming sector size is 512 bytes
#define START_SECTOR 4  // Starting sector for the search
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int copy_in(char *fname) {
    // Open the source file in host filesystem
    FILE *source_file = fopen(fname, "rb");
    if (!source_file) {
        return FILE_DOES_NOT_EXIST;
    }

    // Determine file size
    fseek(source_file, 0, SEEK_END);
    long file_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);

    // Calculate available space and determine write size
    double division = (double)fsutil_freespace() / DIRECT_BLOCKS_COUNT;
    int space_blocks = (int)(division + 1);
    long available_space_bytes =
        fsutil_freespace() * 512 - (space_blocks * 512);
    long write_size = (file_size + 1 < available_space_bytes)
                          ? file_size + 1
                          : available_space_bytes;

    // Create a new file in shell filesystem with the same name
    char *file_name = strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname;
    if (!fsutil_create(file_name, write_size)) {
        fclose(source_file);
        return FILE_CREATION_ERROR;
    }

    // Write the content to the new file
    char *buffer = (char *)malloc(write_size);
    if (buffer == NULL) {
        fclose(source_file);
        return NO_MEM_SPACE;
    }

    fread(buffer, 1, write_size, source_file);
    fsutil_write(file_name, buffer, write_size);

    if (write_size < file_size) {
        // less bytes written than the file size due to limited space
        printf("Warning: could only write %ld out of %ld bytes (reached end of "
               "file).\n",
               write_size, file_size);
    }

    // Clean up
    free(buffer);
    fclose(source_file);
    return 0;
}

int copy_out(char *fname) {
    // Open the source file in shell filesystem
    struct file *source_file = filesys_open(fname);
    if (!source_file) {
        return FILE_DOES_NOT_EXIST;
    }

    // Get file size
    int file_size = file_length(source_file);
    char *buffer = (char *)malloc(file_size);
    if (buffer == NULL) {
        fsutil_close(fname);
        return NO_MEM_SPACE;
    }

    // Ensure reading starts from the beginning
    fsutil_seek(fname, 0);

    // Read the content from the shell file
    int bytes_read = 0;
    char ch;
    while (file_read(source_file, &ch, 1) == 1 && ch != '\0') {
        buffer[bytes_read++] = ch;
    }
    // Write the content to a new file in the host filesystem
    char *file_name = strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname;
    FILE *target_file = fopen(file_name, "wb");
    if (!target_file) {
        free(buffer);
        fsutil_close(fname);
        return FILE_CREATION_ERROR;
    }

    fwrite(buffer, 1, bytes_read, target_file);

    // Clean up
    free(buffer);
    fclose(target_file);
    fsutil_close(fname);

    return 0;
}

/* checks if the given pattern is found within the content of a file */
bool found_in_file(char *pattern, char *fname) {
    // Get file size
    int file_size = fsutil_size(fname);
    char *buffer = malloc(file_size * sizeof(char));
    if (!buffer) {
        printf("Failed to allocate memory for file contents: %s\n", fname);
        return false;
    }

    // Read file
    if (fsutil_read(fname, buffer, file_size) != file_size) {
        printf("Error reading file: %s\n", fname);
        free(buffer);
        return false;
    }

    // Look for pattern
    if (strstr(buffer, pattern) != NULL) {
        // Found pattern, free buffer and close file
        free(buffer);
        return true;
    }

    // Pattern not found, free buffer and close file
    free(buffer);
    return false;
}

void find_file(char *pattern) {
    struct dir *dir;
    char name[NAME_MAX + 1];

    dir = dir_open_root();
    if (dir == NULL)
        return;

    // Search for pattern in every file of the directory
    while (dir_readdir(dir, name)) {
        if (found_in_file(pattern, name))
            printf("%s\n", name);
    }
    dir_close(dir);
}

/* checks if the given inode is fragmented */
bool fragmented_file(struct inode *inode, int print_num_sectors) {
    size_t num_sectors = bytes_to_sectors(inode_length(inode));
    if (print_num_sectors == 0) {
        printf("Num free sectors: %ld\n", num_sectors);
    }

    if (num_sectors <= 1) {
        // single sector cannot be fragmented
        return false;
    }

    block_sector_t *sectors = get_inode_data_sectors(inode);
    bool fragmented = false;

    // Check for any non-continuous sectors (difference greater than 3)
    for (size_t i = 0; i < num_sectors - 1; i++) {
        // Check the condition that defines fragmentation.
        if ((sectors[i + 1] - sectors[i]) > 3) {
            fragmented = true;
            break;
        }
    }

    free(sectors);
    return fragmented;
}

void fragmentation_degree() {
    struct dir *dir;
    char name[NAME_MAX + 1];
    int total_files = 0;
    int fragmented_files = 0;

    dir = dir_open_root();
    if (dir == NULL)
        return;

    int print_num_sectors = 0;
    // Check for fragmented files and count total files
    while (dir_readdir(dir, name)) {
        struct file *file = filesys_open(name);
        if (!file)
            continue;

        struct inode *inode = file_get_inode(file);
        if (!inode) {
            file_close(file);
            continue;
        }
        print_num_sectors += 1;
        if (fragmented_file(inode, print_num_sectors)) {
            fragmented_files++;
        }

        total_files++;
        file_close(file);
    }
    // printf("%d, %d\n", fragmented_files, total_files);
    printf("Num fragmentable files: %d\n", total_files);
    printf("Num fragmented files: %d\n", fragmented_files);
    dir_close(dir);

    // Calculate and print the degree of fragmentation
    if (total_files > 0) {
        double fragmentation_degree = (double)fragmented_files / total_files;
        printf("Fragmentation pct: %.6f\n", fragmentation_degree);
    } else {
        printf("No fragmentable files found.\n");
    }
}

struct FileInfo {
    char *name;
    unsigned size;
    void *content;
    struct FileInfo *next;
};

struct FileInfo *file_list_head = NULL;

struct FileInfo *reverse_list(struct FileInfo *head) {
    struct FileInfo *prev = NULL;
    struct FileInfo *current = head;
    struct FileInfo *next = NULL;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
}

int defragment() {
    struct dir *dir;
    char name[NAME_MAX + 1];
    struct file *file;
    struct inode *inode;

    dir = dir_open_root();
    if (dir == NULL) {
        printf("Error: Cannot open root directory.\n");
        return -1;
    }

    while (dir_readdir(dir, name)) {

        file = filesys_open(name);
        if (!file) {
            continue; // file cannot be opened
        }

        inode = file_get_inode(file);
        if (!inode) {
            file_close(file);
            continue; // inode cannot be retrieved
        }

        size_t file_size = inode_length(inode);
        char *buffer = malloc(file_size);

        if (file_read(file, buffer, file_size) != (int)file_size) {
            printf("Error reading file content.\n");
            free(buffer);
            file_close(file);
            continue;
        }

        // add to linked list
        struct FileInfo *new_file =
            (struct FileInfo *)malloc(sizeof(struct FileInfo));
        new_file->name = strdup(name);
        new_file->size = file_size;
        new_file->content = buffer;
        new_file->next = file_list_head;
        file_list_head = new_file;

        file_close(file);
    }

    dir_close(dir);

    // Remove existing files
    dir = dir_open_root();
    if (dir == NULL) {
        printf("Error: Cannot open root directory.\n");
        return -1;
    }

    while (dir_readdir(dir, name)) {
        fsutil_rm(name);
    }
    dir_close(dir);

    file_list_head = reverse_list(file_list_head);

    struct FileInfo *current = file_list_head;
    while (current != NULL) {
        // printf("%s\n", current->name);
        fsutil_create(current->name, current->size);
        fsutil_write(current->name, current->content, current->size);
        struct FileInfo *temp = current;
        current = current->next;
        free(temp->name);
        free(temp->content);
        free(temp);
    }

    return 0;
}

void recover(int flag) {
    if (flag == 0) { // recover deleted inodes
        static char recovered_name[14];
        struct inode *inode = NULL;

        // Loop through sectors
        for (size_t i = 0; i < bitmap_size(free_map); i++) {
            if (!bitmap_test(free_map, i)) { // Find free sector
                block_sector_t sector = i;
                inode = inode_open(sector);

                if (inode != NULL && inode->data.magic == INODE_MAGIC) { // Check if inode is valid
                    bitmap_flip(free_map, sector);
                    sprintf(recovered_name, "recovered0-%d", (int)sector);
                    // Recover
                    if (!dir_add(dir_open_root(), recovered_name, sector, inode->data.is_dir)) {
                        printf("Failed to add %s\n", recovered_name);
                    }
                    inode_close(inode);
                } else if (inode != NULL) { // magic doesn't match
                    inode_close(inode);
                }
            }
        }

    } else if (flag == 1) { // recover all non-empty sectors
        uint8_t buffer[512];
        char filename[64];
        FILE *fp;

        for (block_sector_t sector = START_SECTOR; sector < bitmap_size(free_map); sector++) {
           // Check if the sector is marked as free
            buffer_cache_read(sector, buffer);
                
            bool is_non_zero = false;
            for (int i = 0; i < 512; i++) {
                if (buffer[i] != 0) {
                    is_non_zero = true;
                    break;
                }
            }

            if (is_non_zero) {
                sprintf(filename, "recovered1-%d.txt", sector);
                fp = fopen(filename, "w");
                if (fp != NULL) {
                    int i = 0;
                    while (buffer[i] != '\0' && i < 512) {
                        fputc(buffer[i], fp);
                        i++;
                    }
                    fclose(fp);
                    //printf("Recovered data to %s\n", filename);
                } else {
                    printf("Failed to open file %s for writing\n", filename);
                }
            }         
        }
        // TODO
    } else if (flag == 2) {
        struct dir *dir = dir_open_root();
        if (!dir) {
            printf("Error: Cannot open root directory.\n");
            return;
        }

        char name[NAME_MAX + 1];
        while (dir_readdir(dir, name)) {
            // Open the file
            struct file *file = filesys_open(name);
            if (!file) {
                continue; // Unable to open file
            }

            struct inode *inode = file_get_inode(file);
            if (!inode) {
                file_close(file);
                continue; // Unable to get inode
            }

            off_t file_size = inode_length(inode);

            size_t num_sectors = bytes_to_sectors(file_size);

            block_sector_t start_sector = inode->sector;

            char buffer[SECTOR_SIZE];
            buffer_cache_read(start_sector + num_sectors - 1, buffer);

            int start_index = file_size % SECTOR_SIZE;
            bool found_hidden_data = false;
            for (int i = start_index; i < SECTOR_SIZE; i++) {
                if (buffer[i] != 0) {
                    found_hidden_data = true;
                    break;
                }
            }

            if (found_hidden_data) {
                char recovered_filename[NAME_MAX + 32];
                sprintf(recovered_filename, "recovered2-%s.txt", name);

                FILE *recovered_file = fopen(recovered_filename, "wb");
                if (!recovered_file) {
                    printf("Error: Unable to create recovered file: %s\n", recovered_filename);
                } else {
                    fwrite(buffer + start_index, sizeof(char), SECTOR_SIZE - start_index, recovered_file);
                    fclose(recovered_file);
                    printf("Recovered hidden data from file: %s\n", name);
                }
            }

            file_close(file);
        }

        dir_close(dir);
    }
}

