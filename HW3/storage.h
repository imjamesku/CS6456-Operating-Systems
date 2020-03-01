#pragma once

#define BLOCK_SIZE 512  // bytes in a block
#define BLOCK_COUNT 256 // blocks in the disk

#define NVM_LEN 60000 // bytes of nonvolatile memory available

int disk_write(int block_number, char *buffer);
int disk_read(int block_number, char *buffer);
int disk_block_count();
int nvm_write(int byte_number, int length, char *buffer);
int nvm_read(int byte_number, int length, char *buffer);
int nvm_byte_count();
