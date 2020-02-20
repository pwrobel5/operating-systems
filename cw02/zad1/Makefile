CC = gcc -Wall -std=c11
output = -o cw02_program.x

all: program

program:
	$(CC) cw02_program.c $(output)

clean:
	rm -f *.x
	rm -f *.b

test:
	$(CC) cw02_program.c $(output)
	./test_script.sh
	rm -f *.x
	rm -f *.b
