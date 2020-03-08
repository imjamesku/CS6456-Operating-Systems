#!/usr/bin/env python3

# Run this like
#
#   ./test_random_files.py

import ctypes
import random
import os
import sys
import time
import matplotlib.pyplot as plt
import numpy as np


def get_lorem_bytes():
    lorem = 'Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do\
	eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\
	veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\
	commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit\
	esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat\
	cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id\
	est laborum.'.split()*10
    file_str = ' '.join(random.choices(
        lorem, k=random.randint(3, 200))).encode('utf-8')
    return file_str

# Create at least 100000 bytes worth of files, and return the number of files
# created.


def create_random_files():
    index = 0
    total_size = 0
    while total_size < 100000:
        fname = "file_{}.txt".format(index).encode('utf-8')

        content = get_lorem_bytes()

        fd = uvafs.uva_open(fname, True)
        ret = uvafs.uva_write(fd, content, len(content))
        if ret == -1:
            return -2
        ret = uvafs.uva_close(fd)
        if ret < 0:
            return -1

        index += 1
        total_size += len(content)

    return index


dir_path = os.path.dirname(os.path.realpath(__file__))

# To use C version, do this:
uvafs = ctypes.CDLL(os.path.join(dir_path, 'libuva_fs.so'))

# To use Python version, create an object called `uvafs` with the filesystem
# functions.
# import uva_fs
# uvafs = uva_fs.filesystem()


number_files = create_random_files()
if number_files < 0:
    print('error creating files')
    sys.exit(-1)

read_times = []

# Do the test five times
for j in range(0, 5):
    for i in range(0, number_files):
        fname = "file_{}.txt".format(i).encode('utf-8')
        fd = uvafs.uva_open(fname, False)

        recv = ctypes.create_string_buffer(50000)

        start = time.time()
        recv_len = uvafs.uva_read(fd, recv, 0, 50000)
        end = time.time()

        read_times.append(end-start)

min_time, max_time = min(read_times), max(read_times)
print(min_time)
print(max_time)
bins = np.arange(min_time, max_time, (max_time-min_time)/100)
print(bins)
plt.hist(read_times, bins=bins, normed=True, cumulative=True,
         label='CDF', histtype='step', alpha=0.8)
plt.xlabel('time')
plt.ylabel('CDF')

plt.show()
# for read_time in read_times:
#     print(read_time)
