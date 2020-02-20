CC = gcc -Wall -std=c11
source_a = cw04_zad01a.c
source_b = cw04_zad01b.c
output_a = cw04_zad01a.x
output_b = cw04_zad01b.x
rm_binaries = rm -f *.x

all: program

program:
	$(CC) $(source_a) -o $(output_a)
	$(CC) $(source_b) -o $(output_b)

clean:
	$(rm_binaries)
