#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/times.h>
#include <signal.h>

#include "cw07_sysV.h"

#define INFINITE_LOOP_ARGS_NUMBER 2
#define FINITE_LOOP_ARGS_NUMBER 3
#define PACK_WEIGHT_POSITION 1
#define CYCLES_NUMBER_POSITION 2

struct Belt* package_belt = NULL;
int shr_mem_id = -1;
int sem_id = -1;
int taken_semaphores[USED_SEMAPHORES];
clock_t start_time = 0;

void handle_sigint(int signum)
{
    printf("\n[%f][PID: %d] SIGINT received, closing loader process...\n", calculate_time_difference(start_time, times(NULL)), getpid());
    exit(EXIT_SUCCESS);
}

void close_loader(void)
{
    if(package_belt != NULL && shmdt(package_belt) != 0)
        raise_error("Error with unconnecting shared memory from loader");

    for(int i = 0; i < USED_SEMAPHORES; i++)
    {
        if(taken_semaphores[i] == 1)
        {
            if(operate_on_semaphore(sem_id, i, SEM_INCR) == -1)
                raise_error("Error with giving back semaphore on closing loader");
        }
    }
}

void parse_initial_arguments(int argc, char* argv[], int* pack_weight, int* cycles_number)
{
    if(argc != INFINITE_LOOP_ARGS_NUMBER && argc != FINITE_LOOP_ARGS_NUMBER)
    {
        errno = EINVAL;
        raise_error("Invalid number of arguments");
    }

    *pack_weight = atoi(argv[PACK_WEIGHT_POSITION]);
    if(*pack_weight == 0)
    {
        errno = EINVAL;
        raise_error("Incorrect weight of package");
    }

    *cycles_number = (argc == INFINITE_LOOP_ARGS_NUMBER) ? -1 : atoi(argv[CYCLES_NUMBER_POSITION]);
    if(*cycles_number == 0)
    {
        errno = EINVAL;
        raise_error("Incorrect number of cycles");
    }
}

void get_shared_mem_access(void)
{
    key_t shr_mem_key = get_ipcs_key();
    if(shr_mem_key == -1)
        exit(EXIT_FAILURE);
    
    shr_mem_id = shmget(shr_mem_key, 0, 0);
    if(shr_mem_id == -1)
        raise_error("Error with getting access to shared memory (maybe trucker is not working)");

    package_belt = shmat(shr_mem_id, NULL, 0);
    if(package_belt == (void *) -1)
        raise_error("Error with getting shared memory address");
}

void get_semaphore_access(void)
{
    key_t sem_key = get_ipcs_key();
    if(sem_key == -1)
        exit(EXIT_FAILURE);
    
    sem_id = semget(sem_key, 0, 0);
    if(sem_id == -1)
        raise_error("Error with getting access to semaphores");
}

int put_on_belt(struct Pack put_pack)
{
    if(package_belt->current_packages == package_belt->max_packages || package_belt->current_weight + put_pack.weight > package_belt->max_weight)
        return -1;
    
    if(package_belt->head == -1)
        package_belt->head = package_belt->tail;
    
    package_belt->packs[package_belt->tail] = put_pack;
    package_belt->tail++;
    if(package_belt->tail == package_belt->max_packages)
        package_belt->tail = 0;
    
    package_belt->current_packages++;
    package_belt->current_weight += put_pack.weight;

    return 0;
}

int main(int argc, char* argv[])
{
    start_time = times(NULL);

    if(atexit(close_loader) != 0)
        raise_error("Error with setting atexit loader function");
        
    struct sigaction loader_sigaction;
    loader_sigaction.sa_handler = handle_sigint;
    sigemptyset(&loader_sigaction.sa_mask);
    loader_sigaction.sa_flags = 0;
    if(sigaction(SIGINT, &loader_sigaction, NULL) != 0)
        raise_error("Error with setting function to handle SIGINT");
    
    int pack_weight, cycles_number;
    parse_initial_arguments(argc, argv, &pack_weight, &cycles_number);
    get_shared_mem_access();
    
    if(pack_weight > package_belt->max_weight)
    {
        printf("[%f][PID: %d] This package is too heave to put it on even empty belt, terminating\n", calculate_time_difference(start_time, times(NULL)), getpid());
        exit(EXIT_FAILURE);
    }
    
    get_semaphore_access();
    
    for(int i = 0; i < USED_SEMAPHORES; i++)
        taken_semaphores[i] = 0;
    
    int working = 1;
    int new_pack = 1;
    struct Pack my_pack;
    my_pack.loaderID = getpid();
    my_pack.weight = pack_weight;

    printf("[%f][PID: %d] Hello, it's loader number %d. Now I'm starting my work\n", calculate_time_difference(start_time, times(NULL)), getpid(), getpid());

    while(working == 1)
    {
        if(operate_on_semaphore(sem_id, LOADER, SEM_DECR) == -1)
            raise_error("Error with taking LOADER semaphore by loader");
        taken_semaphores[LOADER] = 1;
        
        if(operate_on_semaphore(sem_id, BELT, SEM_DECR) == -1)
            raise_error("Error with taking BELT semaphore by loader");
        taken_semaphores[BELT] = 1;

        if(new_pack == 1)
            my_pack.time = get_time_microseconds();

        if(put_on_belt(my_pack) == -1)
        {
            printf("[%f][PID: %d] I can't put my pack on belt, waiting\n", calculate_time_difference(start_time, times(NULL)), my_pack.loaderID);

            if(operate_on_semaphore(sem_id, TRUCKER, SEM_INCR) == -1)   
                raise_error("Error with rising TRUCKER semaphore by loader");
    
            if(operate_on_semaphore(sem_id, BELT, SEM_INCR) == -1)
                raise_error("Error with giving back BELT semaphore by loader");
            taken_semaphores[BELT] = 0;

            new_pack = 0;
        }
        else
        {
            printf("[%f][PID: %d] Successfully put pack on belt, pack weight: %d\n", calculate_time_difference(start_time, times(NULL)), my_pack.loaderID, my_pack.weight);
            new_pack = 1;

            if(operate_on_semaphore(sem_id, BELT, SEM_INCR) == -1)
                raise_error("Error with giving back BELT semaphore by loader");
            taken_semaphores[BELT] = 0;

            if(operate_on_semaphore(sem_id, LOADER, SEM_INCR) == -1)
                raise_error("Error with giving back LOADER semaphore by loader");
            taken_semaphores[LOADER] = 0;

            if(cycles_number != -1)
                cycles_number--;
            
            working = (cycles_number == 0) ? 0 : 1;
        }       
    }

    printf("[%f][PID: %d] I delivered all packs, now it's time to stop\n", calculate_time_difference(start_time, times(NULL)), my_pack.loaderID);    
    return 0;
}