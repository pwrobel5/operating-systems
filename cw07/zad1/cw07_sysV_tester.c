#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "cw07_sysV.h"

#define CORRECT_ARGUMENTS_NUMBER 7
#define LOADERS_NUMBER_POSITION 1
#define MAX_WEIGHT_POSITION 2
#define MAX_CYCLES_POSITION 3
#define TRUCK_MAX_WEIGHT_POSITION 4
#define BELT_MAX_PACKAGES_POSITION 5
#define BELT_MAX_WEIGHT_POSITION 6

#define NUM_AS_CHAR_MAX_LENGTH 6

#define TRUCKER_EXECUTABLE "./cw07_sysV_trucker.x"
#define LOADER_EXECUTABLE "./cw07_sysV_loader.x"

pid_t trucker_pid = -1;

void exit_func(void)
{
    if(trucker_pid != -1)
    {
        kill(trucker_pid, SIGINT);
        waitpid(trucker_pid, NULL, 0);
    }
}

int main(int argc, char* argv[])
{
    if(atexit(exit_func) != 0)
        raise_error("Error setting atexit function");

    srand(time(NULL));

    if(argc != CORRECT_ARGUMENTS_NUMBER)
    {
        errno = EINVAL;
        raise_error("Incorrect number of arguments");
    }

    int loaders_number, max_weight, max_cycles, truck_max_weight, belt_max_packages, belt_max_weight;
    loaders_number = atoi(argv[LOADERS_NUMBER_POSITION]);
    max_weight = atoi(argv[MAX_WEIGHT_POSITION]);
    max_cycles = atoi(argv[MAX_CYCLES_POSITION]);
    truck_max_weight = atoi(argv[TRUCK_MAX_WEIGHT_POSITION]);
    belt_max_packages = atoi(argv[BELT_MAX_PACKAGES_POSITION]);
    belt_max_weight = atoi(argv[BELT_MAX_WEIGHT_POSITION]);
    if(loaders_number == 0 || max_weight == 0 || max_cycles == 0 || truck_max_weight == 0 || belt_max_packages == 0 || belt_max_weight == 0)
    {
        errno = EINVAL;
        raise_error("Incorrect numbers given");
    }

    trucker_pid = fork();
    if(trucker_pid == 0)
    {
        execlp(TRUCKER_EXECUTABLE, TRUCKER_EXECUTABLE, argv[TRUCK_MAX_WEIGHT_POSITION], argv[BELT_MAX_PACKAGES_POSITION], argv[BELT_MAX_WEIGHT_POSITION], NULL);
        raise_error("Error with executing trucker");
    }
    else
    {
        sleep(5);
    }
    
 
    char weight_as_char[NUM_AS_CHAR_MAX_LENGTH];
    char cycles_as_char[NUM_AS_CHAR_MAX_LENGTH];
    pid_t loaders_pids[loaders_number];

    for(int i = 0; i < loaders_number; i++)
    {
        int current_weight = rand() % max_weight + 1;
        int current_cycles = rand() % max_cycles + 10;

        if(sprintf(weight_as_char, "%d", current_weight) < 0)
            raise_error("Error with converting weight to string");
        if(sprintf(cycles_as_char, "%d", current_cycles) < 0)
            raise_error("Error with converting cycles to string");
        
        loaders_pids[i] = fork();
        if(loaders_pids[i] == 0)
        {
            execlp(LOADER_EXECUTABLE, LOADER_EXECUTABLE, weight_as_char, cycles_as_char, NULL);
            raise_error("Error with executing loader");
        }
    }

    for(int i = 0; i < loaders_number; i++)
    {
        waitpid(loaders_pids[i], NULL, 0);
    }
    
    return 0;
}