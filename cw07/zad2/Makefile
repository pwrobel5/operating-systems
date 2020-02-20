CC = gcc
FLAGS = -Wall -std=c11 -o
LINKED = -lrt -lpthread
TRUCKER_SRC = cw07_posix_trucker.c
TRUCKER_OUT = cw07_posix_trucker.x
LOADER_SRC = cw07_posix_loader.c
LOADER_OUT = cw07_posix_loader.x
TESTER_SRC = cw07_posix_tester.c
TESTER_OUT = cw07_posix_tester.x

TESTER_LOADERS = 10
TESTER_MAX_WEIGHT = 12
TESTER_MAX_CYCLES = 10
TESTER_TRUCK_MAX_WEIGHT = 60
TESTER_BELT_MAX_PACKAGES = 10
TESTER_BELT_MAX_WEIGHT = 15

all: trucker loader

trucker:
		$(CC) $(FLAGS) $(TRUCKER_OUT) $(TRUCKER_SRC) $(LINKED)

loader:
		$(CC) $(FLAGS) $(LOADER_OUT) $(LOADER_SRC) $(LINKED)

clean:
		rm -f *.x

test: trucker loader
		$(CC) $(FLAGS) $(TESTER_OUT) $(TESTER_SRC)
		./$(TESTER_OUT) $(TESTER_LOADERS) $(TESTER_MAX_WEIGHT) $(TESTER_MAX_CYCLES) $(TESTER_TRUCK_MAX_WEIGHT) $(TESTER_BELT_MAX_PACKAGES) $(TESTER_BELT_MAX_WEIGHT)
		rm -f *.x