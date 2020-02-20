#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <unistd.h>

#define CORRECT_ARGC_VALUE 3
#define PIPE_NAME_POSITION 1
#define SAVE_COUNT_POSITION 2

#define MAX_LINE_LENGTH 100

int main(int argc, char* argv[])
{
    if(argc != CORRECT_ARGC_VALUE)
    {
        printf("Incorrect arguments number!\n");
        return 1;
    }

    char* pipe_name = argv[PIPE_NAME_POSITION];
    int N = atoi(argv[SAVE_COUNT_POSITION]);

    FILE* fifo_pipe = fopen(pipe_name, "w");
    if(fifo_pipe == NULL)
    {
        printf("Error with opening pipe file!\n");
        return 1;
    }

    srand(time(NULL));

    for(int i = 0; i < N; i++)
    {
        FILE* date_value = popen("date", "r");
        char buffer[MAX_LINE_LENGTH];
        fgets(buffer, MAX_LINE_LENGTH, date_value);
        pclose(date_value);
        
        pid_t slave_pid = getpid();
        printf("Slave PID: %d\n", slave_pid);

        fprintf(fifo_pipe, "%d: %s\n", slave_pid, buffer);

        int sleep_seconds = rand() % 5 + 1;
        sleep(sleep_seconds);        
    }

    fclose(fifo_pipe);

    return 0;
}