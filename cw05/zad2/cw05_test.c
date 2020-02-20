#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define CORRECT_ARGC_VALUE 3
#define RANGE_POSITION 1
#define SLAVES_POSITION 2

#define INT_TO_STRING_LENGTH 5

int main(int argc, char* argv[])
{
    if(argc < CORRECT_ARGC_VALUE)
    {
        printf("Incorrect number of arguments!\n");
        return 1;
    }

    int range = atoi(argv[RANGE_POSITION]);
    int slaves = atoi(argv[SLAVES_POSITION]);

    printf("\n\n***********TESTING CW05 MASTER AND SLAVE**********\n\n");

    srand(time(NULL));
    pid_t master_pid = fork();

    if(master_pid == 0)
    {
        execlp("./cw05_master.x", "./cw05_master.x", "testFifo.fifo", NULL);
        exit(0);
    }

    pid_t createdSlaves[slaves];

    for(int i = 0; i < slaves; i++)
    {
        int number_of_entries = rand() % range + 1;
        char snumber_of_entries[INT_TO_STRING_LENGTH];
        sprintf(snumber_of_entries, "%d", number_of_entries);
        
        createdSlaves[i] = fork();
        if(createdSlaves[i] == 0)
        {
            execl("./cw05_slave.x", "./cw05_slave.x", "testFifo.fifo", snumber_of_entries, NULL);
            exit(0);
        }
    }

    for(int i = 0; i < slaves; i++)
    {
        waitpid(createdSlaves[i], NULL, 0);
        printf("Slave PID %d ended his work\n", createdSlaves[i]);
    }

    kill(master_pid, SIGINT);

    printf("\n\n***********TESTING CW05 MASTER AND SLAVE ENDED**********\n\n");

    return 0;
}