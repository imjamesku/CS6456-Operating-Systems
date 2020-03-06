#include "uva_fs.h"

#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitmap.c"
#include "storage.h"

// #define DEBUG

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

#define BITMAP_ROW_SIZE                                                        \
    (BLOCK_COUNT /                                                             \
     8) // the number of rows we have in the bitmap. we will have 32 rows.

#define NUM_INODES 500
#define ROOTDIR_INODE 0
#define MAGIC 0xdeadbeef
#define MAX_FILE_SIZE BLOCK_SIZE *((BLOCK_SIZE / sizeof(int)) + 12)

// Allows us to keep track of occupied inodes. 1 = occupied, 0 = not
// int inTableStatus[NUM_INODES];

inode_t inTable[NUM_INODES];
file_descriptor fdTable[NUM_INODES];
directory_entry rootDir[NUM_INODES];

bool mounted = false;
superblock_t super;
int indy_block[BLOCK_SIZE / sizeof(int)];

void print_inode(int idx) {
    printf("mode: %d\n", inTable[idx].mode);
    printf("link_cnt: %d\n", inTable[idx].link_cnt);
    printf("uid: %d\n", inTable[idx].uid);
    printf("gid: %d\n", inTable[idx].gid);
    printf("size: %d\n", inTable[idx].size);
    printf("indirectPointer: %d\n", inTable[idx].indirectPointer);
    printf("data:\n");
    for (size_t i = 0; i < 12; i++) {
        printf("%d\n", inTable[idx].data_ptrs[i]);
    }
}

// Sets the values of inode i in the inode table in memory
void set_inode(int i, int mode, int link_cnt, int uid, int gid, int size,
               int data_ptrs[12], int indirectPointer) {
    inTable[i].used = true;
    inTable[i].mode = mode;
    inTable[i].link_cnt = link_cnt;
    inTable[i].uid = uid;
    inTable[i].gid = gid;
    inTable[i].size = size;
    inTable[i].indirectPointer = indirectPointer;
    for (int j = 0; j < 12; j++) {
        inTable[i].data_ptrs[j] = data_ptrs[j];
    }

    // inTableStatus[i] = 1;
}

// The same as set_inode, but instead sets the inode as unused
void rm_inode(int i) {
    inTable[i].used = false;
    inTable[i].mode = -1;
    inTable[i].link_cnt = -1;
    inTable[i].uid = -1;
    inTable[i].gid = -1;
    inTable[i].size = -1;
    inTable[i].indirectPointer = -1;
    inTable[i].uid = -1;

    for (int j = 0; j < 12; j++) {
        inTable[i].data_ptrs[j] = -1;
    }

    // inTableStatus[i] = 0;
}

// Initializes file descriptor table
void init_fdt() {
    for (int i = 0; i < NUM_INODES; i++) {
        fdTable[i].inodeIndex = -1;
    }
}

// Initializes the inode table, by setting all inodes as unused
void init_in_table() {
    for (int i = 0; i < NUM_INODES; i++) {
        rm_inode(i);
    }
}

// Initializes the superblock in memory
void init_super() {
    super.magic = MAGIC;
    super.block_size = BLOCK_SIZE;
    super.fs_size = BLOCK_COUNT;
    super.inode_table_len = NUM_INODES;
    super.root_dir_inode = ROOTDIR_INODE;
}

// Initializes the root directory in memory
void init_root() {
    for (int i = 0; i < NUM_INODES; i++) {
        rootDir[i].num = -1;
        for (int j = 0; j < MAX_FILENAME_LEN; j++) {
            rootDir[i].name[j] = '\0';
        }
    }
}

// Returns first available fd, or -1 if none is available
int first_open_fd() {
    for (int i = 0; i < NUM_INODES; i++) {
        if (fdTable[i].inodeIndex == -1) {
            return i;
        }
    }
    return -1;
}

// Returns first available inode, or -1 if none is available
int first_open_inode() {
    for (int i = 1; i < NUM_INODES; i++) {
        if (inTable[i].used == false) {
            inTable[i].used = true;
            return i;
        }
    }
    return -1;
}

// Returns first available spot in the root dir, or -1 if none is available
int first_open_rootDir() {
    for (int i = 0; i < NUM_INODES; i++) {
        if (rootDir[i].num == -1) {
            return i;
        }
    }
    return -1;
}

int calculate_inode_table_blocks() {
    int numInodeBlocks = (sizeof(inTable) / BLOCK_SIZE);
    if (sizeof(inTable) % BLOCK_SIZE != 0) {
        numInodeBlocks += 1;
    }
    return numInodeBlocks;
}

// Returns the inode corresponding to a given file in the root dir, or -1 if it
// doesn't exist
int retrieve_file(char *path) {
    char *buffer =
        malloc(sizeof(char) *
               (MAX_FILENAME_LEN + 5)); // Slightly bigger than max filename

    for (int i = 0; i < NUM_INODES; i++) {
        if (rootDir[i].num != -1) {
            strcpy(buffer, rootDir[i].name);
            if (strcmp(buffer, path) == 0) {
                free(buffer);
                return rootDir[i].num;
            }
        }
    }
    free(buffer);
    return -1;
}
int write_block(int address, char *buffer) {
    // printf("write address: %d\n", address);
    // printf("byte number: %d\n", address * BLOCK_SIZE);
    // printf("nvm block count: %d\n", NVM_BLOCK_COUNT);
    if (address < NVM_BLOCK_COUNT) {
        nvm_write(address * BLOCK_SIZE, BLOCK_SIZE, buffer);
    } else {
        int disk_address = address - NVM_LEN / BLOCK_SIZE;
        disk_write(disk_address, buffer);
    }
}

int write_blocks(int start_address, int nblocks, char *buffer) {
    for (int i = 0; i < nblocks; i++) {
        write_block(start_address + i, buffer + i * BLOCK_SIZE);
    }
}

int read_block(int address, char *buffer) {
    if (address < NVM_BLOCK_COUNT) {
        nvm_read(address * BLOCK_SIZE, BLOCK_SIZE, buffer);
    } else {
        int disk_address = address - NVM_LEN / BLOCK_SIZE;
        disk_read(disk_address, buffer);
    }
}

int read_blocks(int start_address, int nblocks, void *buffer) {
    char *block_start = (char *)buffer;
    for (int i = 0; i < nblocks; i++) {
        read_block(start_address + i, block_start + i * BLOCK_SIZE);
    }
}

int uva_open(char *filename, bool writeable) {
    // Check for invalid filename
    int count = 0;
    while (*(filename + count) != '\0') {
        count++;
    }
    if (count > MAX_FILENAME_LEN + 1) {
        return -1;
    }
    if (!mounted) {
        D(printf("202\n");)
        read_blocks(0, 1, &super);
        D(printf("204\n");)
        if (super.magic != MAGIC) {
            D(printf("make fresh\n");)
            D(fflush(stdout);)
            uva_make_fs(1);
            mounted = true;
        } else {
            D(printf("make unfresh\n");)
            D(fflush(stdout);)
            uva_make_fs(0);
            mounted = true;
        }
    }
    D("mounted successfully\n";)
    D(fflush(stdout);)

    int newFd = first_open_fd();
    if (newFd != -1) {
        int fileInode = retrieve_file(filename);
        if (fileInode != -1) { // file exists

            for (int i = 0; i < NUM_INODES; i++) {
                if (fdTable[i].inodeIndex == fileInode) {
                    return -1;
                }
            }

            fdTable[newFd].inodeIndex = fileInode;

            // eventually, might want to keep less in active memory...
            fdTable[newFd].inode = &(inTable[fileInode]);

            // We open in append mode, so rwptr is at the end of the file
            // fdTable[newFd].rwptr = inTable[fileInode].size;
            fdTable[newFd].rwptr = 0;

            fdTable[newFd].writeable = writeable;

            return newFd;
        } else { // file does not exist, we need to create it

            // Pick an inode for it
            int newInodeIndex = first_open_inode();
            if (newInodeIndex == -1) { // No more free inodes !
                return -1;
            }
            // Set up the directory entry
            int newDirEntryIndex = first_open_rootDir();
            if (newDirEntryIndex == -1) { // No more open spots in rootDir
                return -1;
            }

            // Pick a data block for it
            int newDataPtr = get_index();
            if (newDataPtr == 0) {
                return -1;
            }

            int data_ptrs[12];
            data_ptrs[0] = newDataPtr;
            for (int i = 1; i < 12; i++) {
                data_ptrs[i] = -1;
            }

            fdTable[newFd].inodeIndex = newInodeIndex;

            rootDir[newDirEntryIndex].num = newInodeIndex;
            strcpy(rootDir[newDirEntryIndex].name, filename);

            // Set up the inode
            set_inode(newInodeIndex, 0, 1, 0, 0, 0, data_ptrs, -1);
            fdTable[newFd].inode = &(inTable[newInodeIndex]);
            fdTable[newFd].rwptr = inTable[newInodeIndex].size;
            fdTable[newFd].writeable = writeable;

            int numRootDirBlocks = (sizeof(rootDir) / BLOCK_SIZE);
            if (sizeof(rootDir) % BLOCK_SIZE != 0) {
                numRootDirBlocks += 1;
            }

            inTable[ROOTDIR_INODE].size += 1;

            void *bufferino = malloc(BLOCK_SIZE * numRootDirBlocks);
            memcpy(bufferino, &rootDir, sizeof(rootDir));
            // Write rootDir to disk
            write_blocks(inTable[ROOTDIR_INODE].data_ptrs[0], numRootDirBlocks,
                         (char *)bufferino);
            free(bufferino);

            // Write the inode table to disk
            int numInodeBlocks = calculate_inode_table_blocks();
            write_blocks(1, numInodeBlocks, (char *)&inTable);

            // Write the inode status to disk
            // write_blocks(1022, 1, (char *)&inTableStatus);

            // Write bitmap to disk
            write_blocks(1023, 1, (char *)&free_bit_map);
            return newFd;
        }
    } else {
        return -1;
    }
}

int uva_close(int file_identifier) {
    if (fdTable[file_identifier].inodeIndex == -1) {
        return -1;
    } else {
        fdTable[file_identifier].inodeIndex = -1;
        return 0;
    }
}
// TODO: add support for offset
int uva_read(int file_identifier, char *buffer, int offset, int length) {

    int inodeRD = fdTable[file_identifier].inodeIndex;
    int toRead;
    int end;
    D(printf("read--------\n");)
    D(printf("fd: %d\n", file_identifier);)
    D(printf("rwptr: %d\n", fdTable[file_identifier].rwptr);)
    D(printf("inoderd: %d\n", inodeRD);)
    D(print_inode(inodeRD);)
    // Handle "wrong input" situations
    if (file_identifier < 0) {
        return -1;
    } // not a valid fileID
    if (inodeRD == -1) {
        return -1;
    } // no corresponding fd
    if (fdTable[file_identifier].writeable) {
        return -1;
    } // Not in read mode
    if (inTable[inodeRD].size <= 0) {
        // return length;
        return -1;
    } // nothing to read

    if (inTable[inodeRD].size <
        (fdTable[file_identifier].rwptr +
         length)) { // we're asked to read too much for the size of the file
        toRead = inTable[inodeRD].size - fdTable[file_identifier].rwptr;
        end = inTable[inodeRD].size / BLOCK_SIZE;
        if ((inTable[inodeRD].size % BLOCK_SIZE) != 0) {
            end += 1;
        }
    } else {
        toRead = length;
        end = (fdTable[file_identifier].rwptr + length) / BLOCK_SIZE;
        if ((fdTable[file_identifier].rwptr + length) % BLOCK_SIZE != 0) {
            end += 1;
        }
    }

    // Calculate index of first block
    int start = fdTable[file_identifier].rwptr / BLOCK_SIZE;
    // Calculate offset
    int startOffset = fdTable[file_identifier].rwptr % BLOCK_SIZE;

    // If the indirect pointer is used, we'll need additional addresses
    void *temp_buffer = malloc(BLOCK_SIZE);
    if (inTable[inodeRD].link_cnt > 12) {
        read_blocks(inTable[inodeRD].indirectPointer, 1, temp_buffer);
        memcpy(&indy_block, temp_buffer, BLOCK_SIZE);
    }

    // Create a buffer to store the file
    void *bufferino = malloc(BLOCK_SIZE * end);
    D(printf("start: %d\n", start);)
    D(printf("end: %d\n", end);)
    // Load up the file in memory; first the direct pointers, then the indirect
    for (int i = start; i < inTable[inodeRD].link_cnt && i < end; i++) {
        if (i >= 12) {
            read_blocks(indy_block[i - 12], 1,
                        (bufferino + (i - start) * BLOCK_SIZE));
        } else {
            read_blocks(inTable[inodeRD].data_ptrs[i], 1,
                        (bufferino + (i - start) * BLOCK_SIZE));
        }
    }
    D(printf("bufferino: %s\n", bufferino);)
    D(printf("toRead: %d\n", toRead);)
    // Copy the part we want to buf
    memcpy(buffer, (bufferino + startOffset), toRead);
    D(printf("buffer: %s", buffer);)

    // Set the rwptr to its new home
    fdTable[file_identifier].rwptr += toRead;

    free(bufferino);
    free(temp_buffer);

    return toRead;
}

int uva_read_reset(int file_identifier) {
    return uva_fseek(file_identifier, 0);
}

int uva_write(int file_identifier, char *buffer, int length) {
    int inodeWR = fdTable[file_identifier].inodeIndex;
    int toWrite = length;

    // Handle "wrong input" situations
    if (file_identifier < 0) {
        return -1;
    } // not a valid file_identifier
    if (inodeWR == -1) {
        return -1;
    } // no corresponding fd
    if (fdTable[file_identifier].writeable == false) {
        return -1;
    }

    // If the write doesn't fit in the currently allocated space, we'll need to
    // free up more
    int bytesNeeded = fdTable[file_identifier].rwptr + length;
    if (bytesNeeded > MAX_FILE_SIZE) {
        bytesNeeded = MAX_FILE_SIZE;
        toWrite = MAX_FILE_SIZE - fdTable[file_identifier].rwptr;
    }

    int currentBlocks = inTable[inodeWR].link_cnt;

    int blocksNeeded = bytesNeeded / BLOCK_SIZE;
    if (bytesNeeded % BLOCK_SIZE != 0) {
        blocksNeeded += 1;
    }

    int additionalBlocks = blocksNeeded - currentBlocks;
    D(printf("additional blocks: %d\n", additionalBlocks);)

    // We might need the indirect pointer
    void *temp_buffer = malloc(BLOCK_SIZE);
    if (inTable[inodeWR].link_cnt > 12) {
        read_blocks(inTable[inodeWR].indirectPointer, 1, temp_buffer);
        memcpy(&indy_block, temp_buffer, BLOCK_SIZE);
    } else if (inTable[inodeWR].link_cnt + additionalBlocks > 12) {
        int data_ptr = get_index();
        if (data_ptr == 0) {
            return -1;
        }
        inTable[inodeWR].indirectPointer = data_ptr;
    }
    free(temp_buffer);

    if (additionalBlocks > 0) {
        // Set aside space for the new bytes
        for (int i = inTable[inodeWR].link_cnt;
             i < inTable[inodeWR].link_cnt + additionalBlocks; i++) {
            int new_datablock = get_index();
            if (new_datablock ==
                0) { // We don't have enough blocks and need to abort
                // Better handling needed here for large overflows?
                // Since this behavior is undefined, I simply cancelled the
                // write
                return -1;
            } else {
                if (i >= 12) {
                    indy_block[i - 12] = new_datablock;
                } else {
                    inTable[inodeWR].data_ptrs[i] = new_datablock;
                }
            }
        }
    } else {
        additionalBlocks = 0;
    }

    // Find the starting and ending block, and the startOffset
    int start = fdTable[file_identifier].rwptr / BLOCK_SIZE;
    int startOffset = fdTable[file_identifier].rwptr % BLOCK_SIZE;

    D(printf("start: %d\n", start);)
    D(printf("startOffset: %d\n", startOffset);)

    int end = blocksNeeded;

    // Now load up the current file
    void *bufferino = malloc(BLOCK_SIZE * blocksNeeded);

    for (int i = start; i < inTable[inodeWR].link_cnt && i < end; i++) {
        if (i >= 12) {
            read_blocks(indy_block[i - 12], 1,
                        (bufferino + (i - start) * BLOCK_SIZE));
        } else {
            read_blocks(inTable[inodeWR].data_ptrs[i], 1,
                        (bufferino + (i - start) * BLOCK_SIZE));
        }
    }

    memcpy((bufferino + startOffset), buffer, toWrite);

    // Write the inode status to disk
    // write_blocks(1022, 1, (char *)&inTableStatus);

    // Write the bufferino back to disk
    for (int i = start; i < end; i++) {
        if (i >= 12) {
            write_blocks(indy_block[i - 12], 1,
                         (char *)(bufferino + (i - start) * BLOCK_SIZE));
        } else {
            write_blocks(inTable[inodeWR].data_ptrs[i], 1,
                         (char *)(bufferino + ((i - start) * BLOCK_SIZE)));
        }
    }

    // Save the changes we made to the file system

    // Update the inode
    D(printf("bytes needed:%d\n", bytesNeeded);)
    D(printf("inodeWR: %d\n", inodeWR);)
    if (inTable[inodeWR].size < bytesNeeded) {
        inTable[inodeWR].size = bytesNeeded;
    }

    inTable[inodeWR].link_cnt += additionalBlocks;
    fdTable[file_identifier].rwptr = bytesNeeded;

    if (inTable[inodeWR].link_cnt > 12) {
        write_blocks(inTable[inodeWR].indirectPointer, 1, (char *)&indy_block);
    }

    // Write the inode table to disk
    int numInodeBlocks = calculate_inode_table_blocks();
    write_blocks(1, numInodeBlocks, (char *)&inTable);

    // Write the inode status to disk
    // write_blocks(1022, 1, (char *)&inTableStatus);

    // Write bitmap to disk
    write_blocks(1023, 1, (char *)&free_bit_map);

    free(bufferino);
    D(printf("write---------\n");)
    D(print_inode(inodeWR);)
    return toWrite;
}

void uva_make_fs(int fresh) {
    if (fresh == 1) {

        init_fdt();

        init_root();

        init_in_table();

        // Calculate the number of blocks occupied by the inode table
        int numInodeBlocks = calculate_inode_table_blocks();

        // Calculate the number of blocks occupied by the root directory
        int numRootDirBlocks = (sizeof(rootDir) / BLOCK_SIZE);
        if (sizeof(rootDir) % BLOCK_SIZE != 0) {
            numRootDirBlocks += 1;
        }

        // Set aside space on the bitmap for superblock, inode table, root dir
        // free bitmap and inodeTableStatus
        force_set_index(0);
        for (int i = 1; i < numInodeBlocks + 1; i++) {
            force_set_index(
                i); // offset by one, because our superblock is in block 0
        }

        for (int i = numInodeBlocks + 1;
             i < numRootDirBlocks + (numInodeBlocks + 1); i++) {
            force_set_index(i);
        }

        force_set_index(1022);
        force_set_index(1023);

        int rootptrs[1024];
        for (int i = 0; i < numRootDirBlocks; i++) {
            rootptrs[i] = i + numInodeBlocks + 1;
        }

        set_inode(ROOTDIR_INODE, 0, numRootDirBlocks, 0, 0, -1, rootptrs, -1);

        // Write superblock to disk
        init_super();
        write_blocks(0, 1, (char *)&super);

        // Write inode table to disk
        write_blocks(1, numInodeBlocks, (char *)inTable);

        void *bufferino = malloc(BLOCK_SIZE * numRootDirBlocks);
        memcpy(bufferino, &rootDir, sizeof(rootDir));
        // Write rootDir to disk
        write_blocks(inTable[ROOTDIR_INODE].data_ptrs[0], numRootDirBlocks,
                     (char *)bufferino);
        free(bufferino);

        // Write status of inode table to disk
        // write_blocks(1022, 1, (char *)&inTableStatus);

        // Write bitmap to disk
        write_blocks(1023, 1, (char *)&free_bit_map);

    } else {

        init_fdt();

        void *bufferino = malloc(BLOCK_SIZE);

        // Read the superblock into memory
        read_blocks(0, 1, bufferino);
        memcpy(&super, bufferino, sizeof(superblock_t));

        // Read the inode table into memory
        // Calculate the number of blocks occupied by the inode table
        int numInodeBlocks = calculate_inode_table_blocks();

        free(bufferino);

        bufferino = malloc(BLOCK_SIZE * numInodeBlocks);
        read_blocks(1, numInodeBlocks, bufferino);
        memcpy(&inTable, bufferino, sizeof(inTable));

        free(bufferino);

        // Read the inode table status into memory
        // bufferino = malloc(BLOCK_SIZE);
        // read_blocks(1022, 1, bufferino);
        // memcpy(&inTableStatus, bufferino, sizeof(inTableStatus));

        // free(bufferino);

        // Read the bitmap into memory
        bufferino = malloc(BLOCK_SIZE);
        read_blocks(1023, 1, bufferino);
        memcpy(&free_bit_map, bufferino, sizeof(free_bit_map));

        free(bufferino);

        // Read the root directory into memory
        bufferino = malloc(BLOCK_SIZE * (inTable[ROOTDIR_INODE].link_cnt));
        read_blocks(inTable[ROOTDIR_INODE].data_ptrs[0],
                    inTable[ROOTDIR_INODE].link_cnt, bufferino);
        memcpy(&rootDir, bufferino, sizeof(rootDir));

        free(bufferino);
    }
}

int uva_getnextfilename(char *filename) { return -1; }

int uva_getfilesize(const char *path) { return -1; }

int uva_fseek(int file_identifier, int loc) {
    // As a precaution, to avoid invalid seeks
    if (inTable[fdTable[file_identifier].inodeIndex].size < loc) {
        return -1;
    }

    fdTable[file_identifier].rwptr = loc;
    return 0;
}

int uva_remove(char *file) { return -1; }
