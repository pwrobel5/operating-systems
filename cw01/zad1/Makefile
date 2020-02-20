CC = gcc -Wall -std=c11

all: static shared clean

static:
	$(CC) -c cw01_lib.c
	ar rcs libcw01_lib.a cw01_lib.o

shared:
	$(CC) -c -fPIC cw01_lib.c
	$(CC) -shared -Wl,-soname,libshcw01_lib.so -o libshcw01_lib.so cw01_lib.o -lc

clean:
	rm -f *.o

clean_binaries:
	rm -f *.a
	rm -f *.so.*

test: static
	$(CC) -c test.c
	$(CC) test.o libcw01_lib.a -o test.x
	./test.x
	rm -f test.x
	rm -f *.o
	rm -f *.a
