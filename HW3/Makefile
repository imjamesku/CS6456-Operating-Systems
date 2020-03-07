
all: storage.c storage.h uva_fs.c uva_fs.h
	gcc -fPIC -shared -o libuva_fs.so storage.c uva_fs.c
mytest:
	gcc -o 
clean:
	rm uva_fs mytest

.PHONY: clean