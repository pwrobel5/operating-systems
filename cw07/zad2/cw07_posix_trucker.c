#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>

#include "cw07_posix.h"

#define CORRECT_ARGUMENTS_NUMBER 4
#define TRUCK_MAX_WEIGHT_POSITION 1
#define BELT_MAX_PACKAGES_POSITION 2
#define BELT_MAX_WEIGHT_POSITION 3

struct Belt* package_belt = NULL;
int truck_max_weight = 0;
int loaded_packs_weight = 0;

int shr_mem_id = -1;
sem_t* trucker_sem = NULL;
sem_t* belt_sem = NULL;
sem_t* loader_sem = NULL;

clock_t start_time = 0;

int take_from_belt(struct Pack* taken_pack)
{
    if(package_belt->head == -1)
        return -1;
    
    *taken_pack = package_belt->packs[package_belt->head];
    package_belt->head++;

    if(package_belt->head == package_belt->max_packages)
        package_belt->head = 0;

    if(package_belt->head == package_belt->tail)
        package_belt->head = -1;
    
    package_belt->current_packages--;
    package_belt->current_weight -= taken_pack->weight;

    return 0; 
}

void unpack_belt(void)
{
    struct Pack taken_pack;
    while(take_from_belt(&taken_pack) == 0)
    {
        if(taken_pack.weight + loaded_packs_weight > truck_max_weight)
        {
            printf("[%f] Truck is full and departs\n", calculate_time_difference(start_time, times(NULL)));
            sleep(TRUCK_UNLOAD_TIME);
            loaded_packs_weight = 0;
            printf("[%f] Empty truck has arrived, waiting for packs\n", calculate_time_difference(start_time, times(NULL)));
        }

        loaded_packs_weight += taken_pack.weight;
        printf("[%f] Pack received to truck\n\tfrom worker %d\n\tit needed %ld μs to get to the truck\n\tweight of packs in truck: %d\n\tpacks on belt: %d\n\tweight left in truck: %d\n",
                calculate_time_difference(start_time, times(NULL)), taken_pack.loaderID, get_time_microseconds() - taken_pack.time, 
                loaded_packs_weight, package_belt->current_packages, truck_max_weight - loaded_packs_weight);            
    }
}

void handle_sigint(int signum)
{
    printf("\n[%f] SIGINT received, closing trucker process...\n", calculate_time_difference(start_time, times(NULL)));
    exit(EXIT_SUCCESS);
}

void clean_mem(void)
{
    if(sem_close(trucker_sem) == -1)
        raise_error("Error with closing TRUCKER semaphore");
    
    if(sem_close(belt_sem) == -1)
        raise_error("Error with closing BELT semaphore");
    
    if(sem_close(loader_sem) == -1)
        raise_error("Error with closing LOADER semaphore");

    if(sem_unlink(TRUCKER_SEMAPHORE_NAME) == -1)
        raise_error("Error with unlinking TRUCKER semaphore");
    
    if(sem_unlink(BELT_SEMAPHORE_NAME) == -1)
        raise_error("Error with unlinking BELT semaphore");
    
    if(sem_unlink(LOADER_SEMAPHORE_NAME) == -1)
        raise_error("Error with unlinking LOADER semaphore");

    if(package_belt != NULL)
    {
        if(package_belt->current_packages != 0)
            unpack_belt();
    }

    if(shr_mem_id >= 0)
    {
        if(munmap(package_belt, sizeof(struct Belt)) == -1)
            raise_error("Error with unlinking sharing memory");
        
        if(shm_unlink(SHARED_MEMORY_NAME) == -1)
            raise_error("Error with deleting shared memory");
    }
}

void parse_initial_arguments(int argc, char* argv[], int* belt_max_packages, int* belt_max_weight)
{
    if(argc != CORRECT_ARGUMENTS_NUMBER)
    {
        errno = EINVAL;
        raise_error("Invalid number of arguments");
    }

    truck_max_weight = atoi(argv[TRUCK_MAX_WEIGHT_POSITION]);
    *belt_max_packages = atoi(argv[BELT_MAX_PACKAGES_POSITION]);
    *belt_max_weight = atoi(argv[BELT_MAX_WEIGHT_POSITION]);
}

void initialize_shared_memory(int belt_max_packages, int belt_max_weight)
{
    shr_mem_id = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, SHARED_OBJECTS_PERMISSIONS);
    if(shr_mem_id == -1)
        raise_error("Error with creating shared memory segment");
    
    if(ftruncate(shr_mem_id, sizeof(struct Belt)) == -1)
        raise_error("Error with setting shared memory size");
    
    package_belt = mmap(NULL, sizeof(struct Belt), PROT_READ | PROT_WRITE, MAP_SHARED, shr_mem_id, 0);
    if(package_belt == MAP_FAILED)
        raise_error("Error with getting shared memory address");
    
    package_belt->current_packages = 0;
    package_belt->current_weight = 0;
    package_belt->max_packages = belt_max_packages;
    package_belt->max_weight = belt_max_weight;
    package_belt->head = -1;
    package_belt->tail = 0;
}

void initialize_semaphores(void)
{
    trucker_sem = sem_open(TRUCKER_SEMAPHORE_NAME, O_CREAT | O_EXCL, SHARED_OBJECTS_PERMISSIONS, 0);
    if(trucker_sem == SEM_FAILED)
        raise_error("Error creating TRUCKER semaphore");
    
    belt_sem = sem_open(BELT_SEMAPHORE_NAME, O_CREAT | O_EXCL, SHARED_OBJECTS_PERMISSIONS, 1);
    if(belt_sem == SEM_FAILED)
        raise_error("Error creating BELT semaphore");

    loader_sem = sem_open(LOADER_SEMAPHORE_NAME, O_CREAT | O_EXCL, SHARED_OBJECTS_PERMISSIONS, 1);
    if(loader_sem == SEM_FAILED)
        raise_error("Error creating LOADER semaphore");
}

int main(int argc, char* argv[])
{
    start_time = times(NULL);

    if(atexit(clean_mem) != 0)
        raise_error("Error with setting atexit function");

    struct sigaction trucker_sigaction;
    trucker_sigaction.sa_handler = handle_sigint;
    sigemptyset(&trucker_sigaction.sa_mask);
    trucker_sigaction.sa_flags = 0;
    if(sigaction(SIGINT, &trucker_sigaction, NULL) != 0)
        raise_error("Error with setting function to handle SIGINT");

    int belt_max_packages, belt_max_weight;
    parse_initial_arguments(argc, argv, &belt_max_packages, &belt_max_weight);    
    initialize_shared_memory(belt_max_packages, belt_max_weight);
    initialize_semaphores();

    printf("[%f] Starting simulation\n", calculate_time_difference(start_time, times(NULL)));
    
    int working = 1;

    while(working == 1)
    {
        if(sem_wait(trucker_sem) == -1)
            raise_error("Error with taking TRUCKER semaphore by trucker");

        if(sem_wait(belt_sem) == -1)
            raise_error("Error with taking BELT semaphore by trucker");

        unpack_belt();

        if(sem_post(belt_sem) == -1)
            raise_error("Error with giving back BELT semaphore");

        if(sem_post(loader_sem) == -1)
            raise_error("Error with giving back LOADERS semaphore by trucker");
    }

    return 0;
}