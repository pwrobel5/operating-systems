CC = gcc -Wall -std=c11
source_catcher = cw04_catcher.c
source_sender = cw04_sender.c
binary_catcher = cw04_catcher.x
binary_sender = cw04_sender.x
rm_binaries = rm -rf *.x
test_script_file = test_script.sh

all: program

program:
	$(CC) $(source_catcher) -o $(binary_catcher)
	$(CC) $(source_sender) -o $(binary_sender)

clean:
	$(rm_binaries)

test:
	$(CC) $(source_catcher) -o $(binary_catcher)
	$(CC) $(source_sender) -o $(binary_sender)
	./$(test_script_file)
	$(rm_binaries)