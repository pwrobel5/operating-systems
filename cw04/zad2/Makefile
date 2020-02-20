CC = gcc -Wall -std=c11
source = cw04_monitor.c
tester = cw04_tester.c
tester_bin = cw04_tester.x
tester_out = -o cw04_tester.x
output = -o cw04_monitor.x
rm_binaries = rm -f *.x
rm_archive = rm -r archive -f
rm_test_files = rm -r *.test
test_script = test_script.sh

all: program

program:
	$(CC) $(source) $(output)

clean:
	$(rm_binaries)
	$(rm_archive)
	$(rm_test_files)

test:
	$(CC) $(source) $(output)
	$(CC) $(tester) $(tester_out)
	./$(test_script)
	$(rm_binaries)
	$(rm_archive)
	$(rm_test_files)

