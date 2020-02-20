CC = gcc -Wall -std=c11

CREATE_ARRAY = create_table 10

FIND_SMALL_DIR = search_directory /usr/ intellij* find_small.txt
FIND_MEDIUM_DIR = search_directory /usr/ in* find_medium.txt
FIND_BIG_DIR = search_directory /usr/ r* find_large.txt

REMOVE_FIRST_SEGMENT = remove_block 0
REMOVE_SECOND_SEGMENT = remove_block 1
REMOVE_THIRD_SEGMENT = remove_block 2

OUTPUT_NAME_STATIC = wyniki3s.txt
OUTPUT_NAME_SHARED = wyniki3sh.txt
OUTPUT_NAME_DYNAMIC = wyniki3d.txt

O_OUTPUT = Ores.txt
O1_OUTPUT = O1res.txt
OS_OUTPUT = Osres.txt
FULL_OUTPUT = results3b.txt

all: static shared dynamic clean

static:
	$(CC) -c ../zad2/cw01_program.c
	$(CC) cw01_program.o -o cw01_program_static.x -L ../zad1 -l cw01_lib -ldl

shared:
	$(CC) -c ../zad2/cw01_program.c
	$(CC) cw01_program.o -L ../zad1 -l shcw01_lib -o cw01_program_shared.x -ldl
	export LD_LIBRARY_PATH=../zad1/

dynamic:
	$(CC) -c ../zad2/cw01_program.c
	$(CC) cw01_program.o -L ../zad1 -ldl -l shcw01_lib -o cw01_program_dynamic.x

clean:
	rm -f *.o

clean_binaries:
	rm -f *.x

test: test_o test_o1 test_os
	echo "O level" > $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(O_OUTPUT) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "O1 level" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(O1_OUTPUT) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Os level" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OS_OUTPUT) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	rm -f O*res.txt

test_o:
	$(CC) -O -c ../zad2/cw01_program.c
	$(CC) -O cw01_program.o -o cw01_program_static.x -L ../zad1 -l cw01_lib -ldl
	rm -f *.o
	./cw01_program_static.x $(OUTPUT_NAME_STATIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -O -c ../zad2/cw01_program.c
	$(CC) -O cw01_program.o -L ../zad1 -l shcw01_lib -o cw01_program_shared.x -ldl
	export LD_LIBRARY_PATH=../zad1/
	rm -f *.o
	./cw01_program_shared.x $(OUTPUT_NAME_SHARED) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -O -c ../zad2/cw01_program.c
	$(CC) -O cw01_program.o -L ../zad1 -ldl -l shcw01_lib -o cw01_program_dynamic.x
	rm -f *.o
	./cw01_program_dynamic.x $(OUTPUT_NAME_DYNAMIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	rm -f *.x
	rm -f find*.txt

	echo "Static library:" > $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_STATIC) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Shared library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_SHARED) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Dynamic library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_DYNAMIC) >> $(O_OUTPUT)

	rm -f wyniki*.txt

test_o1:
	$(CC) -O1 -c ../zad2/cw01_program.c
	$(CC) -O1 cw01_program.o -o cw01_program_static.x -L ../zad1 -l cw01_lib -ldl
	rm -f *.o
	./cw01_program_static.x $(OUTPUT_NAME_STATIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -O1 -c ../zad2/cw01_program.c
	$(CC) -O1 cw01_program.o -L ../zad1 -l shcw01_lib -o cw01_program_shared.x -ldl
	export LD_LIBRARY_PATH=../zad1/
	rm -f *.o
	./cw01_program_shared.x $(OUTPUT_NAME_SHARED) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -O1 -c ../zad2/cw01_program.c
	$(CC) -O1 cw01_program.o -L ../zad1 -ldl -l shcw01_lib -o cw01_program_dynamic.x
	rm -f *.o
	./cw01_program_dynamic.x $(OUTPUT_NAME_DYNAMIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	rm -f *.x
	rm -f find*.txt

	echo "Static library:" > $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_STATIC) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Shared library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_SHARED) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Dynamic library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_DYNAMIC) >> $(O1_OUTPUT)

	rm -f wyniki*.txt

test_os:
	$(CC) -Os -c ../zad2/cw01_program.c
	$(CC) -Os cw01_program.o -o cw01_program_static.x -L ../zad1 -l cw01_lib -ldl
	rm -f *.o
	./cw01_program_static.x $(OUTPUT_NAME_STATIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -Os -c ../zad2/cw01_program.c
	$(CC) -Os cw01_program.o -L ../zad1 -l shcw01_lib -o cw01_program_shared.x -ldl
	export LD_LIBRARY_PATH=../zad1/
	rm -f *.o
	./cw01_program_shared.x $(OUTPUT_NAME_SHARED) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	$(CC) -Os -c ../zad2/cw01_program.c
	$(CC) -Os cw01_program.o -L ../zad1 -ldl -l shcw01_lib -o cw01_program_dynamic.x
	rm -f *.o
	./cw01_program_dynamic.x $(OUTPUT_NAME_DYNAMIC) $(CREATE_ARRAY) $(FIND_SMALL_DIR) $(FIND_MEDIUM_DIR) $(FIND_BIG_DIR) $(REMOVE_FIRST_SEGMENT) $(REMOVE_SECOND_SEGMENT) $(REMOVE_THIRD_SEGMENT)

	rm -f *.x
	rm -f find*.txt

	echo "Static library:" > $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_STATIC) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Shared library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_SHARED) >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	echo "Dynamic library" >> $(FULL_OUTPUT)
	echo "" >> $(FULL_OUTPUT)
	cat $(OUTPUT_NAME_DYNAMIC) >> $(OS_OUTPUT)

	rm -f wyniki*.txt
