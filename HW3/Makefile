
all: storage.c storage.h uva_fs.c uva_fs.h
	gcc -fPIC -shared -o libuva_fs.so storage.c uva_fs.c
mytest:
	gcc -o mytest mytest.c storage.c uva_fs.c
clean:
	rm libuva_fs.so mytest

.PHONY: clean