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
    double division = (double)fsutil_freespace()/DIRECT_BLOCKS_COUNT;
    int space_blocks = (int)(division + 1);
    long available_space_bytes =  fsutil_freespace()*512 - (space_blocks*512);
    long write_size =
        (file_size+1 < available_space_bytes) ? file_size+1 : available_space_bytes;
   
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
bool fragmented_file(struct inode *inode) {
    size_t num_sectors = bytes_to_sectors(inode_length(inode));
    printf("Num free sectors: %ld\n", num_sectors);
    if (num_sectors <= 1) {
        // single sector cannot be fragmented
        return false;
    }

    block_sector_t *sectors = get_inode_data_sectors(inode);
    //printf("sectors: %ls\n", sectors);
    bool fragmented = false;

    // Check for any non-continuous sectors (difference greater than 3)
    for (size_t i = 0; i < num_sectors - 1; i++) {
        // Check the condition that defines fragmentation.
        //printf("sectors i+1: %d, sectors i: %d\n", sectors[i + 1], sectors[i]);
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

        if (fragmented_file(inode)) {
            fragmented_files++;
        }

        total_files++;
        file_close(file);
    }
    printf("%d, %d\n", fragmented_files, total_files);
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

        // Store file contents into memory
        size_t file_size = inode_length(inode);
        char *buffer = malloc(file_size);
        if (buffer == NULL) {
            printf("Error: Cannot allocate memory for file contents.\n");
            file_close(file);
            dir_close(dir);
            return -1;
        }

        if (file_read(file, buffer, file_size) != file_size) {
            printf("Error: Could not read complete file.\n");
            free(buffer);
            file_close(file);
            continue;
        }

        file_close(file);

        // Delete and recreate the file, which defragments it
        if (!filesys_remove(name)) {
            printf("Error: Failed to remove file during defragmentation.\n");
            free(buffer);
            continue;
        }

        if (!filesys_create(name, file_size, inode_is_directory(inode))) {
            printf("Error: Failed to create file during defragmentation.\n");
            free(buffer);
            continue;
        }

        file = filesys_open(name);
        if (!file) {
            printf("Error: Failed to open newly created file.\n");
            free(buffer);
            continue;
        }

        if (file_write(file, buffer, file_size) != file_size) {
            printf("Error: Could not write complete file.\n");
            free(buffer);
            file_close(file);
            continue;
        }

        file_close(file);
        free(buffer);
    }

    dir_close(dir);
    return 0;
}

void recover(int flag) {
    if (flag == 0) { // recover deleted inodes

        // TODO
    } else if (flag == 1) { // recover all non-empty sectors

        // TODO
    } else if (flag == 2) { // data past end of file.

        // TODO
    }
}