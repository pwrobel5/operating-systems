CC = gcc -Wall -std=c11
source = cw03_monitor.c
tester = cw03_tester.c
tester_bin = cw03_tester.x
tester_out = -o cw03_tester.x
output = -o cw03_monitor.x
rm_binaries = rm -f *.x
rm_archive = rm -r archive -f
test_script = test_script.sh

all: program

program:
	$(CC) $(source) $(output)

clean:
	$(rm_binaries)
	$(rm_archive)

test:
	$(CC) $(source) $(output)
	$(CC) $(tester) $(tester_out)
	./$(test_script)
	$(rm_binaries)
	$(rm_archive)

