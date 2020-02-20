#ifndef CW07_POSIX_HEADER
#define CW07_POSIX_HEADER

#include <stdio.h>

#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>
#include <time.h>

#define MAX_BELT_CAPACITY 512

#define SHARED_MEMORY_NAME "/cw07sharedmem"
#define TRUCKER_SEMAPHORE_NAME "/cw07truckersem"
#define BELT_SEMAPHORE_NAME "/cw07beltsem"
#define LOADER_SEMAPHORE_NAME "/cw07loadersem"

#define USED_SEMAPHORES_LOADER 3

#define SHARED_OBJECTS_PERMISSIONS S_IRWXU | S_IRWXG | S_IRWXO

#define TRUCK_UNLOAD_TIME 3

enum sem_type {
    TRUCKER, BELT, LOADER
};

struct Pack {
    int weight;
    pid_t loaderID;
    long time;
};

struct Belt {
    struct Pack packs[MAX_BELT_CAPACITY];
    int current_weight;
    int current_packages;
    int max_weight;
    int max_packages;
    int head;
    int tail;
};

void raise_error(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

long get_time_microseconds()
{
    struct timespec tspec;
    if(clock_gettime(CLOCK_MONOTONIC, &tspec) != 0)
        raise_error("Error with reading time");

    return (tspec.tv_nsec / 1000);
}

double calculate_time_difference(clock_t from, clock_t to)
{
    return (double) (to - from) / sysconf(_SC_CLK_TCK);
}

#endif