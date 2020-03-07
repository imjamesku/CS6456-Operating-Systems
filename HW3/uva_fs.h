#pragma once

#include <stdbool.h>

#define NUM_FILES 500

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

// reset the whole filesystem
void reset();