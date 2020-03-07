#!/usr/bin/env python3

# Run this like
#
#   ./test_small_files.py <number>
#
# where <number> is the number of files to create. This will print the number of
# seconds to create that many files after initializing the filesystem.
#
# Note that for measuring your filesystem you need to ensure that everything
# is reset (including the storage files) before running this script.

import ctypes
import os
import sys
import time
import matplotlib.pyplot as plt


# Called to ensure the filesystem is initialized


def init():
    fname = "init_file.txt".encode('utf-8')
    content = b'c'

    fd = uvafs.uva_open(fname, True)
    ret = uvafs.uva_write(fd, content, 1)
    if ret == -1:
        return -2
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

# Create a certain number of 256 byte files.


def create_small_files(number_files):
    for i in range(0, number_files):
        fname = "small_file_{}.txt".format(i).encode('utf-8')
        content = b'9'*256

        fd = uvafs.uva_open(fname, True)
        ret = uvafs.uva_write(fd, content, 256)
        if ret == -1:
            return -2
        ret = uvafs.uva_close(fd)
        if ret < 0:
            return -1

    return 0


dir_path = os.path.dirname(os.path.realpath(__file__))
numbers_of_files = list(range(0, 500, 10))
print(numbers_of_files)
durations = []
for number_of_files in numbers_of_files:
    uvafs = ctypes.CDLL(os.path.join(dir_path, 'libuva_fs.so'))
    uvafs.init()
    init()
    start = time.time()
    ret = create_small_files(number_of_files)
    end = time.time()
    if ret != 0:
        print('Error creating {} small files: {}'.format(number_of_files, ret))
    duration = end-start
    print(duration)
    durations.append(duration)

print(durations)

plt.plot(numbers_of_files, durations)
plt.xlabel('number of files')
plt.ylabel('duration')
plt.show()
