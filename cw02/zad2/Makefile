CC = gcc -Wall -std=c11
output = -o cw02_directories.x

all: program

program:
	$(CC) cw02_directories.c $(output)

clean:
	rm -f *.x

test:
	$(CC) cw02_directories.c $(output)
	./test_script.sh
	rm -f *.x

