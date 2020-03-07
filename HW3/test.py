import ctypes
import os
import random


def get_lorem_bytes():
    lorem = 'Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do\
	eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim\
	veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea\
	commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit\
	esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat\
	cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id\
	est laborum.'.split()
    file_str = ' '.join(random.choices(
        lorem, k=random.randint(3, 50))).encode('utf-8')
    return file_str


def test_01_create_file():
    '''
    Create a file and write to it.
    '''
    fd = uvafs.uva_open("test01.txt".encode('utf-8'), True)
    if fd < 0:
        return -1
    file_str = b"this is my file."
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret < 0:
        return -2
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -3

    return 0


def test_02_create_read_file():
    '''
    Create a file, write to it, and then verify it can be read back properly.
    '''
    fd = uvafs.uva_open("test02.txt".encode('utf-8'), True)
    if fd < 0:
        return -1
    file_str = b"this is my file."
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret < 0:
        return -1
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    fd = uvafs.uva_open("test02.txt".encode('utf-8'), False)
    if fd < 0:
        return -1
    recv = ctypes.create_string_buffer(512)
    recv_len = uvafs.uva_read(fd, recv, 0, 512)
    if recv_len < 0:
        return -1
    if recv_len != len(file_str):
        print('str:{}'.format(file_str))
        print('recv:{}'.format(recv))
        print('recv_len: {}'.format(recv_len))
        print('len: {}'.format(len(file_str)))
        return -2
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    if file_str != recv[0:len(file_str)]:
        return -3

    return 0


def test_03_longer_file():
    '''
    Create a file that is a bit longer.
    '''
    fd = uvafs.uva_open("test03.txt".encode('utf-8'), True)
    if fd < 0:
        return -1
    file_str = get_lorem_bytes()
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret < 0:
        return -1
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    return 0


def test_04_longer_file_rw():
    '''
    Create a file that is a bit longer.
    '''
    fd = uvafs.uva_open("test04.txt".encode('utf-8'), True)
    if fd < 0:
        return -1
    file_str = get_lorem_bytes()
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret < 0:
        return -1
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    fd = uvafs.uva_open("test04.txt".encode('utf-8'), False)
    if fd < 0:
        return -1
    recv = ctypes.create_string_buffer(len(file_str))
    recv_len = uvafs.uva_read(fd, recv, 0, len(file_str))
    if recv_len < 0:
        return -1
    if recv_len != len(file_str):
        return -2
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    if file_str != recv.raw:
        return -3

    return 0


def test_05_long_file_name():
    '''
    Create a file that is a bit longer.
    '''
    fd = uvafs.uva_open(
        "this_is_my_really_long_file_name and it has spaces too that shouldn't be a problem right? wow ok 127 characters is actually qui".encode('utf-8'), True)
    if fd < 0:
        return -1
    file_str = b"very long filename."
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret < 0:
        return -1
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    return 0


def test_06_write_a_read_file():
    '''
    Create a file that is a bit longer.
    '''
    success = False
    fd = uvafs.uva_open("test06.txt".encode('utf-8'), False)
    if fd < 0:
        return -1
    file_str = b"very long filename."
    ret = uvafs.uva_write(fd, file_str, len(file_str))
    if ret == -1:
        success = True
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    if success:
        return 0
    else:
        return -2


def test_07_read_a_write_file():
    '''
    Create a file that is a bit longer.
    '''
    success = False
    fd = uvafs.uva_open("test07.txt".encode('utf-8'), True)
    if fd < 0:
        return -1
    recv = ctypes.create_string_buffer(512)
    recv_len = uvafs.uva_read(fd, recv, 0, 512)
    if recv_len == -1:
        success = True
    ret = uvafs.uva_close(fd)
    if ret < 0:
        return -1

    if success:
        return 0
    else:
        return -2


def run_test(test):
    print('Running test "{}"...'.format(test.__name__), end='')
    ret = test()
    if ret == 0:
        print('SUCCESS')
    else:
        print('ERROR: {}'.format(ret))


dir_path = os.path.dirname(os.path.realpath(__file__))

# To use C version, do this:
uvafs = ctypes.CDLL(os.path.join(dir_path, 'libuva_fs.so'))

# To use Python version, create an object called `uvafs` with the filesystem
# functions.
# import uva_fs
# uvafs = uva_fs.filesystem()


run_test(test_01_create_file)
run_test(test_02_create_read_file)
# run_test(test_03_longer_file)
# run_test(test_04_longer_file_rw)
# run_test(test_05_long_file_name)
# run_test(test_06_write_a_read_file)
# run_test(test_07_read_a_write_file)
