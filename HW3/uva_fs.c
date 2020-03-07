#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "uva_fs.h"

enum mode { unspecified, read, write };
typedef struct inode {
    char filename[256];
    bool in_use; // Indicates if the inode contains a file
    int block;
    int size; // size in bytes
    int cursor;
    int start_nvm;
    int nvm_size; // size in bytes stored in nvm
} inode;

typedef struct fileDescriptor {
    bool in_use;
    int inode_idx;
    enum mode open_mode; // 0 unspecified, 1 read, 2 write
    int cursor;
} fileDescriptor;

inode file_list[500];
fileDescriptor fd_table[NUM_FILES];
int file_cnt = 0;
int block = 0;
bool initialized = false;
int nvm = 0;

void init_file_list() {
    for (int i = 0; i < file_cnt; i++) {
        file_list[i].in_use = false;
        file_list[i].block = -1;
        file_list[i].size = -1;
        file_list[i].cursor = -1;
        file_list[i].start_nvm = -1;
        file_list[i].nvm_size = -1;
    }
}

void init_fd_table() {
    for (int i = 0; i < NUM_FILES; i++) {
        fd_table[i].in_use = false;
        fd_table[i].inode_idx = -1;
        fd_table[i].open_mode = 0;
        fd_table[i].cursor = 0;
    }
}

void init() {
    // printf("initializing\n");
    init_file_list();
    init_fd_table();
    file_cnt = 0;
    block = 0;
    nvm = 0;
    initialized = true;
}

int get_next_fd() {
    for (int i = 0; i < NUM_FILES; i++) {
        if (fd_table[i].in_use == false) {
            return i;
        }
    }
}

int get_next_inode_idx() {
    for (int i = 0; i < NUM_FILES; i++) {
        if (file_list[i].in_use == false) {
            return i;
        }
    }
}

int get_next_id() { return file_cnt++; }

int get_next_block() { return block++; }

bool has_block(int block_num, int *block) {
    static int block_used = 0;
    if (block_used + block_num > disk_block_count()) {
        return false;
    }
    *block = block_used;
    block_used += block_num;
    return true;
}

int uva_open(char *filename, bool writeable) {
    if (!initialized) {
        init();
    }
    // printf("opening %s\n",filename);
    // inode *f = NULL;
    int i = 0;
    int inode_idx = -1;
    for (int i = 0; i < NUM_FILES; i++) {
        if (strcmp(file_list[i].filename, filename) == 0) {
            inode_idx = i;
            for (int j = 0; j < NUM_FILES; j++) {
                if (fd_table[j].in_use && fd_table[j].inode_idx == inode_idx) {
                    // file already opened
                    return -1;
                }
            }
            break;
        }
    }
    if (inode_idx == -1) {
        // File does not exist. Need to create.
        // printf("a new file\n");
        inode_idx = get_next_inode_idx();
        strcpy(file_list[inode_idx].filename, filename);
        file_list[i].block = -1;
        file_list[i].size = -1;
        file_list[i].start_nvm = -1;
        // file_list[f->file_identifier] = f;
    }
    // Set up fd
    int file_descriptor = get_next_fd();
    fd_table[file_descriptor].in_use = true;
    if (writeable) {
        fd_table[file_descriptor].open_mode = write;
    } else {
        fd_table[file_descriptor].open_mode = read;
    }
    fd_table[file_descriptor].cursor = 0;
    fd_table[file_descriptor].inode_idx = inode_idx;
    fd_table[file_descriptor].cursor = 0;
    fd_table[file_descriptor].in_use = true;
    // printf("assigned %d\n",f->file_identifier);
    return file_descriptor;
}

int uva_close(int file_identifier) {
    if (fd_table[file_identifier].in_use == false) {
        return -1;
    }
    fd_table[file_identifier].in_use = false;
    fd_table[file_identifier].cursor = 0;
    fd_table[file_identifier].inode_idx = -1;
    fd_table[file_identifier].open_mode = 0;

    return 0;
}

int uva_read(int file_identifier, char *buffer, int offset, int length) {
    // printf("reading file '%s' size: %d\n",f->filename,f->nvm_size+f->size);
    if (fd_table[file_identifier].in_use == false ||
        fd_table[file_identifier].open_mode != read) {
        // can't read
        return -1;
    }
    int inode_idx = fd_table[file_identifier].inode_idx;
    int len = length;
    int i = 0;
    int ret = 0;
    int start = offset + fd_table[file_identifier].cursor;

    inode *f = &file_list[inode_idx];

    if (start >= f->size + f->nvm_size) {
        // printf("skipped the whole file.\n");
        return 0;
    }
    // read from nvm
    if (f->start_nvm != -1) {
        if (start >= f->nvm_size) {
            start -= f->nvm_size;
            // printf("skip nvm\n");
        } else {
            int read_len = f->nvm_size - start;
            if (read_len > length) {
                read_len = length;
            }
            if (-1 == nvm_read(f->start_nvm + start, read_len, buffer)) {
                printf("read error\n");
                return -1;
            }
            ret += read_len;
            // printf("read %d bytes from nvm.\n", ret);
            if (read_len == length) {
                f->cursor += read_len;
                return length;
            }
            length -= read_len;
            start = 0;
        }
    }
    if (f->block != -1) {
        // printf("read from disk.\n");
        int s = 0;
        int start_block = f->block;
        while (s + 512 < start) {
            s += 512;
            start_block++;
        }
        offset = start - s;
        int m = 0;
        while (len > 0) {
            char mem[512] = {0};
            // printf("read from block %d\n",start_block+i);
            if (-1 == disk_read(start_block + i, mem)) {
                // read failed
                return -1;
            }
            bool finish = false;
            // printf("read content: '%s'\n",mem);
            for (int j = offset; j < 512 && len > 0; j++) {
                if (mem[j] == 0) {
                    finish = true;
                    break;
                }
                buffer[ret] = mem[j];
                ret++;
                m++;
                len--;
            }
            if (finish) {
                break;
            }
            offset = 0;
            i++;
        }
    }
    // printf("%d byte read\n",ret);
    f->cursor += ret;
    return ret;
}

int uva_read_reset(int file_identifier) {
    if (fd_table[file_identifier].in_use == false) {
        return -1;
    }
    fd_table[file_identifier].cursor = 0;
    return 0;
}

int uva_write(int file_identifier, char *buffer, int length) {
    if (fd_table[file_identifier].in_use == false ||
        fd_table[file_identifier].open_mode != write) {
        // file cannot be written
        return -1;
    }
    inode *f = &file_list[fd_table[file_identifier].inode_idx];
    int s = 0;
    // see if there are space in nvm
    if (nvm < nvm_byte_count()) {
        // store into nvm
        // printf("write into nvm\n");
        f->start_nvm = nvm;
        int space = nvm_byte_count() - nvm;
        int cnt = 0;
        if (space > length) {
            cnt = length;
        } else {
            cnt = space;
        }
        if (-1 != nvm_write(nvm, cnt, buffer)) {
            nvm += cnt;
            f->nvm_size = cnt;
            if (cnt == length) {
                // printf("nvm is enough.\n");
                // printf("%d bytes written\n",cnt);
                return 0;
            }
            s = cnt;
            length -= cnt;
        }
    }
    int block_num = 0;
    block_num = length / 512;
    if (length % 512 != 0) {
        block_num++;
    }
    int block = 0;
    if (!has_block(block_num, &block)) {
        // no space for write
        return -1;
    }
    // printf("block %d assigned\n",block);
    f->block = block;
    int size = 0;
    for (int i = f->block; i < f->block + block_num; i++) {
        char buff[512] = {0};
        for (int j = 0; j < 512 && buffer[(i - f->block) * 512 + j + s] != 0;
             j++) {
            buff[j] = buffer[(i - f->block) * 512 + j + s];
            size++;
        }
        // printf("write to block %d",i);
        if (-1 == disk_write(i, buff)) {
            // printf("\tfailed\n");
            return -1;
        }
        // printf("\tsuccess\n");
    }
    // printf("%d byte written.\n",size);
    f->size = size;
    return 0;
}

void reset() { initialized = false; }
