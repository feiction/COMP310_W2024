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

    // Check available space in shell filesystem
    if (fsutil_freespace() < file_size) {
        fclose(source_file);
        return NO_MEM_SPACE;
    }

    // Create a new file in shell filesystem with the same name
    char *file_name = strrchr(fname, '/') ? strrchr(fname, '/') + 1 : fname;
    if (!fsutil_create(file_name, file_size)) {
        fclose(source_file);
        return FILE_CREATION_ERROR;
    }

    // Write the content to the new file
    char *buffer = (char *)malloc(file_size);
    fread(buffer, 1, file_size, source_file);
    int bytes_written = fsutil_write(file_name, buffer, file_size);
    if (bytes_written < file_size) {
        printf("Warning: could only write %d out of %ld bytes (reached end of "
               "file).\n",
               bytes_written, file_size);
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
    int bytes_read = fsutil_read(fname, buffer, file_size);

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

void find_file(char *pattern) {
    // TODO
    return;
}

void fragmentation_degree() {
    // TODO
}

int defragment() {
    // TODO
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