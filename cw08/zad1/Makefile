CC = gcc
OUTPUT_NAME = cw08_filter.x
SOURCE_NAME = cw08_filter.c
CFLAGS = -Wall -g -o $(OUTPUT_NAME)
LIBS = -lpthread -lm

all: cw08_filter.c
	$(CC) $(CFLAGS) $(SOURCE_NAME) $(LIBS)

clean:
	rm -f *.x
	rm -f *out.ascii.pgm

test: all
	./run_tests.sh
	rm -f *.x
