#ifndef CW07_SYSV_HEADER
#define CW07_SYSV_HEADER

#define _GNU_SOURCE

#include <stdio.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>

#define MAX_BELT_CAPACITY 512

#define FTOK_VARIABLE "HOME"
#define FTOK_LETTER 'u'
#define SHARED_OBJECTS_PERMISSIONS 0666

#define USED_SEMAPHORES 3
#define SEM_UP 1
#define SEM_DOWN 0
#define SEM_INCR 1
#define SEM_DECR -1

#define TRUCK_UNLOAD_TIME 3

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

enum semaphore_name {
    TRUCKER, BELT, LOADER
};

void raise_error(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

key_t get_ipcs_key(void)
{
    char* ftok_path = getenv(FTOK_VARIABLE);
    if(ftok_path == NULL)
        raise_error("$HOME variable does not exist");
        
    key_t result = ftok(ftok_path, FTOK_LETTER);
    if(result == -1)
        raise_error("Error with generating shared memory key");
    
    return result;
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

int operate_on_semaphore(int sem_id, int sem_num, int operation)
{
    struct sembuf sbuf;
    sbuf.sem_num = sem_num;
    sbuf.sem_op = operation;
    sbuf.sem_flg = 0;

    return semop(sem_id, &sbuf, 1);    
}

#endif