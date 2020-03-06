#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MAX_FILENAME_LEN 256

typedef struct superblock_t {
    uint64_t magic;
    uint64_t block_size;
    uint64_t fs_size;
    uint64_t inode_table_len;
    uint64_t root_dir_inode;
} superblock_t;

typedef struct inode_t {
    bool used;
    unsigned int mode;
    unsigned int link_cnt;
    unsigned int uid;
    unsigned int gid;
    unsigned int size;
    unsigned int data_ptrs[12];
    unsigned int indirectPointer; // points to a data block that points to other
                                  // data blocks (Single indirect)
} inode_t;

/*
 * inodeIndex    which inode this entry describes
 * inode  pointer towards the inode in the inode table
 *rwptr    where in the file to start
 */
typedef struct file_descriptor {
    uint64_t inodeIndex;
    inode_t *inode; //
    uint64_t rwptr;
    bool writeable;
} file_descriptor;

typedef struct directory_entry {
    int num; // represents the inode number of the entery.
    char name[MAX_FILENAME_LEN + 1]; // represents the name of the entery.
} directory_entry;

// Open a file and return a file identifier.
int uva_open(char *filename, bool writeable);

// Close a file based on the identifier.
int uva_close(int file_identifier);

// Read the next `length` bytes, skipping an optional offset.
int uva_read(int file_identifier, char *buffer, int offset, int length);

// Reset the read position to the beginning of the file.
int uva_read_reset(int file_identifier);

// Write the buffer to the end of the file.
int uva_write(int file_identifier, char *buffer, int length);

// helper functions
void uva_make_fs(int fresh);

int uva_getnextfilename(char *filename);

int uva_getfilesize(const char *path);

int uva_fseek(int file_identifier, int loc);

int uva_remove(char *file);
