#include "uva_fs.h"
#include <stdio.h>
#include <string.h>
int main() {
    int fd = uva_open("test01.txt", true);
    if (fd < 0) {
        printf("open failed\n");
        return -1;
    }
    char file_str[] = "this is my file.";
    int ret = uva_write(fd, file_str, strlen(file_str));
    if (ret < 0) {
        printf("write failed\n");
        return -2;
    }

    ret = uva_close(fd);
    if (ret < 0) {
        printf("close failed\n");
        return -3;
    }
    return 0;
}