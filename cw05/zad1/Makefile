CC = gcc -Wall -std=c11
rm_binaries = rm -f *.x
source = cw05_pot_nienazwane.c
output = cw05_pot_nienazwane.x
test_script = test_script.sh

all: program

program:
	$(CC) $(source) -o $(output)

clean:
	$(rm_binaries)

test:
	$(CC) $(source) -o $(output)
	./$(test_script)
	$(rm_binaries)