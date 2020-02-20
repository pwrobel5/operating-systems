CC = gcc -Wall -std=c11

CREATE_ARRAY = create_table 10

FIND_SMALL_DIR = search_directory /usr/ intellij* find_small.txt
FIND_MEDIUM_DIR = search_directory /usr/ in* find_medium.txt
FIND_BIG_DIR = search_directory /usr/ r* find_large.txt

REMOVE_FIRST_SEGMENT = remove_block 0
REMOVE_SECOND_SEGMENT = remove_block 1
REMOVE_THIRD_SEGMENT = remove_block 2

OUTPUT_NAME = raport2.txt

all: static clean

static:
	$(CC) -c cw01_program.c
	$(CC) cw01_program.o -o cw01_program.x -L ../zad1 -l cw01_lib -ldl

clean:
	rm -f *.o

clean_binaries:
	rm -f *.x

test:
	$(CC) -c cw01_program.c
	$(CC) cw01_program.o -o cw01_program.x -L ../zad1 -l cw01_lib -ldl
	rm -f *.o
	./cw01_program.x $(OUTPUT_NAME) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)
	rm -f *.x
	rm -f find*.txt
