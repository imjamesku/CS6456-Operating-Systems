#include "uva_fs.h"

#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "storage.h"

#define DEBUG

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

int uva_open(char *filename, bool writeable) { return -1; }

int uva_close(int file_identifier) { return -1; }

int uva_read(int file_identifier, char *buffer, int offset, int length) {

    return -1;
}

int uva_read_reset(int file_identifier) { return -1; }

int uva_write(int file_identifier, char *buffer, int length) { return -1; }
