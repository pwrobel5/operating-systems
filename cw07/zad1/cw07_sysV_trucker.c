#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/times.h>
#include <signal.h>

#include "cw07_sysV.h"

#define CORRECT_ARGUMENTS_NUMBER 4
#define TRUCK_MAX_WEIGHT_POSITION 1
#define BELT_MAX_PACKAGES_POSITION 2
#define BELT_MAX_WEIGHT_POSITION 3

struct Belt* package_belt = NULL;
int shr_mem_id = -1;
int sem_id = -1;
int truck_max_weight = 0;
int loaded_packs_weight = 0;
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

void clean_ipc(void)
{
    if(sem_id >= 0)
    {
        if(semctl(sem_id, 0, IPC_RMID) == -1)
            raise_error("Error with deleting semaphores set");
    }

    if(package_belt != NULL)
    {
        if(package_belt->current_packages != 0)
            unpack_belt();

        if(shmdt(package_belt) != 0)
            raise_error("Error with deallocating shared memory");
    }

    if(shr_mem_id >= 0)
    {
        if(shmctl(shr_mem_id, IPC_RMID, NULL) != 0)
            raise_error("Error with deleting shared memory segment");
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
    key_t shr_mem_key = get_ipcs_key();
    if(shr_mem_key == -1)
        exit(EXIT_FAILURE);

    shr_mem_id = shmget(shr_mem_key, sizeof(struct Belt), IPC_CREAT | IPC_EXCL | SHARED_OBJECTS_PERMISSIONS);
    if(shr_mem_id == -1)
        raise_error("Error with creating shared memory segment");

    package_belt = shmat(shr_mem_id, NULL, 0);
    if(package_belt == (void *) -1)
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
    key_t sem_key = get_ipcs_key();
    if(sem_key == -1)
        exit(EXIT_FAILURE);
    
    sem_id = semget(sem_key, USED_SEMAPHORES, IPC_CREAT | IPC_EXCL | SHARED_OBJECTS_PERMISSIONS);
    if(sem_id == -1)
        raise_error("Error with creating semaphores set");

    if(semctl(sem_id, TRUCKER, SETVAL, SEM_DOWN) == -1)
        raise_error("Error with initializing TRUCKER semaphore");
    if(semctl(sem_id, BELT, SETVAL, SEM_UP) == -1)
        raise_error("Error with initializing BELT semaphore");
    if(semctl(sem_id, LOADER, SETVAL, SEM_UP) == -1)
        raise_error("Error with initializing LOADER semaphore");
}

int main(int argc, char* argv[])
{
    start_time = times(NULL);

    if(atexit(clean_ipc) != 0)
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
        if(operate_on_semaphore(sem_id, TRUCKER, SEM_DECR) == -1)
            raise_error("Error with taking TRUCKER semaphore by trucker");

        if(operate_on_semaphore(sem_id, BELT, SEM_DECR) == -1)
            raise_error("Error with taking BELT semaphore by trucker");

        unpack_belt();

        if(operate_on_semaphore(sem_id, BELT, SEM_INCR) == -1)
            raise_error("Error with giving back BELT semaphore");

        if(operate_on_semaphore(sem_id, LOADER, SEM_INCR) == -1)
            raise_error("Error with giving back LOADER semaphore by trucker");        
    }

    return 0;
}