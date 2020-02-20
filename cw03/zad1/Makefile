CC = gcc -Wall -std=c11
source = cw03_directories.c
output = -o cw03_directories.x
rm_binaries = rm -f *.x

all: program

program:
	$(CC) $(source) $(output)

clean:
	$(rm_binaries)

test:
	$(CC) $(source) $(output)
	./test_script.sh
	$(rm_binaries)

