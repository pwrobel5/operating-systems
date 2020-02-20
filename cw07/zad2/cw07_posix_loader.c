#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/times.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>

#include "cw07_posix.h"

#define INFINITE_LOOP_ARGS_NUMBER 2
#define FINITE_LOOP_ARGS_NUMBER 3
#define PACK_WEIGHT_POSITION 1
#define CYCLES_NUMBER_POSITION 2

struct Belt* package_belt = NULL;

int shr_mem_id = -1;
sem_t* trucker_sem = NULL;
sem_t* belt_sem = NULL;
sem_t* loader_sem = NULL;
int taken_semaphores[USED_SEMAPHORES_LOADER];

clock_t start_time = 0;

void handle_sigint(int signum)
{
    printf("\n[%f][PID: %d] SIGINT received, closing loader process...\n", calculate_time_difference(start_time, times(NULL)), getpid());
    exit(EXIT_SUCCESS);
}

void close_loader(void)
{
    if(package_belt != NULL && munmap(package_belt, sizeof(struct Belt)) == -1)
        raise_error("Error with unconnecting shared memory from loader");

    if(taken_semaphores[BELT] == 1 && sem_post(belt_sem) == -1)
        raise_error("Error with giving back BELT semaphore");
    
    if(taken_semaphores[LOADER] == 1 && sem_post(loader_sem) == -1)
        raise_error("Error with giving back LOADER semaphore");

    if(sem_close(trucker_sem) == -1)
        raise_error("Error with closing TRUCKER semaphore");
    
    if(sem_close(belt_sem) == -1)
        raise_error("Error with closing BELT semaphore");
    
    if(sem_close(loader_sem) == -1)
        raise_error("Error with closing LOADER semaphore");
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
    shr_mem_id = shm_open(SHARED_MEMORY_NAME, O_RDWR, SHARED_OBJECTS_PERMISSIONS);
    if(shr_mem_id == -1)
        raise_error("Error with getting access to shared memory (maybe trucker is not working");
    
    if(ftruncate(shr_mem_id, sizeof(struct Belt)) == -1)
        raise_error("Error with setting shared memory size");
    
    package_belt = mmap(NULL, sizeof(struct Belt), PROT_READ | PROT_WRITE, MAP_SHARED, shr_mem_id, 0);
    if(package_belt == MAP_FAILED)
        raise_error("Error with getting shared memory address");
}

void get_semaphore_access(void)
{
    trucker_sem = sem_open(TRUCKER_SEMAPHORE_NAME, 0);
    if(trucker_sem == SEM_FAILED)
        raise_error("Error creating TRUCKER semaphore");
    
    belt_sem = sem_open(BELT_SEMAPHORE_NAME, 0);
    if(belt_sem == SEM_FAILED)
        raise_error("Error creating BELT semaphore");

    loader_sem = sem_open(LOADER_SEMAPHORE_NAME, 0);
    if(loader_sem == SEM_FAILED)
        raise_error("Error creating LOADER semaphore");
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
    for(int i = 0; i < USED_SEMAPHORES_LOADER; i++)
        taken_semaphores[i] = 0;
    
    int working = 1;
    int new_pack = 1;
    struct Pack my_pack;
    my_pack.loaderID = getpid();
    my_pack.weight = pack_weight;

    printf("[%f][PID: %d] Hello, it's loader number %d. Now I'm starting my work\n", calculate_time_difference(start_time, times(NULL)), getpid(), getpid());

    while(working == 1)
    {
        if(sem_wait(loader_sem) == -1)
            raise_error("Error with taking LOADER semaphore by loader");
        taken_semaphores[LOADER] = 1;
        
        if(sem_wait(belt_sem) == -1)
            raise_error("Error with taking BELT semaphore by loader");
        taken_semaphores[BELT] = 1;

        if(new_pack == 1)
            my_pack.time = get_time_microseconds();

        if(put_on_belt(my_pack) == -1)
        {
            printf("[%f][PID: %d] I can't put my pack on belt, waiting\n", calculate_time_difference(start_time, times(NULL)), my_pack.loaderID);

            if(sem_post(trucker_sem) == -1)   
                raise_error("Error with rising TRUCKER semaphore by loader");
    
            if(sem_post(belt_sem) == -1)
                raise_error("Error with giving back BELT semaphore by loader");
            taken_semaphores[BELT] = 0;
            new_pack = 0;           
        }
        else
        {
            printf("[%f][PID: %d] Successfully put pack on belt, pack weight: %d\n", calculate_time_difference(start_time, times(NULL)), my_pack.loaderID, my_pack.weight);
            new_pack = 1;

            if(sem_post(belt_sem) == -1)
                raise_error("Error with giving back BELT semaphore by loader");
            taken_semaphores[BELT] = 0;

            if(sem_post(loader_sem) == -1)
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