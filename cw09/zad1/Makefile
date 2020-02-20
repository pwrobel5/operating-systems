CC = gcc
SOURCE_NAME = cw09_roller_coaster.c
OUTPUT_NAME = cw09_roller_coaster.x
CFLAGS = -Wall -o $(OUTPUT_NAME)
LIBS = -lpthread

all: cw09_roller_coaster.c
	$(CC) $(CFLAGS) $(SOURCE_NAME) $(LIBS)

clean:
	rm -f *.x

test: all
	./$(OUTPUT_NAME) 15 3 4 5
	rm -f *.x