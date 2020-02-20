#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>

#define CORRECT_ARGC_VALUE 2
#define PIPE_NAME_POSITION 1

#define MAX_LINE_LENGTH 100

int main(int argc, char* argv[])
{
    if(argc != CORRECT_ARGC_VALUE)
    {
        printf("Incorrect number of arguments!\n");
        return 1;
    }

    char* pipe_name = argv[PIPE_NAME_POSITION];
    char buffer[MAX_LINE_LENGTH];

    mkfifo(pipe_name, S_IRUSR | S_IWUSR);

    FILE* fifo_pipe = fopen(pipe_name, "r");
    if(fifo_pipe == NULL)
    {
        printf("Error with creating fifo pipe!\n");
        return 1;
    }

    while(1)
    {
        if(fgets(buffer, MAX_LINE_LENGTH, fifo_pipe) != NULL)
        {
            printf("%s\n", buffer);
        }
    }

    fclose(fifo_pipe);

    return 0;
}