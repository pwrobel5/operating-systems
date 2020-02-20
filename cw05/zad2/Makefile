CC = gcc -Wall -std=c11
rm_binaries = rm -f *.x
source_master = cw05_master.c
source_slave = cw05_slave.c
output_master = cw05_master.x
output_slave = cw05_slave.x
source_test = cw05_test.c
output_test = cw05_test.x

all: program

program:
	$(CC) $(source_master) -o $(output_master)
	$(CC) $(source_slave) -o $(output_slave)

clean:
	$(rm_binaries)

test:
	$(CC) $(source_master) -o $(output_master)
	$(CC) $(source_slave) -o $(output_slave)
	$(CC) $(source_test) -o $(output_test)
	./$(output_test) 4 5
	$(rm_binaries)
	rm -f *.fifo