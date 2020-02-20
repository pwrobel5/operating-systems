#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CORRECT_ARGC_NUMBER 2
#define FILE_NAME_ARGUMENT_POSITION 1

#define MAX_LINE_LENGTH 200
#define ARGS_SEPARATOR " "
#define PIPE_SEPARATOR "|"
#define DELIMITERS {'\n', '\t', ' '}
#define NUM_OF_DELIMITERS 3

#define PIPE_ARRAY_SIZE 2
#define PIPE_READ_INDEX 0
#define PIPE_WRITE_INDEX 1

int count_lines(char* input_file_name)
{
    int lines_number = 0;
    char buffer[MAX_LINE_LENGTH];
    char* tmp;
    FILE* input_file = fopen(input_file_name, "r");
    if(input_file == NULL)
    {
        printf("Error with opening input file!\n");
        return -1;
    }

    while(fgets(buffer, MAX_LINE_LENGTH, input_file) != NULL)
    {
        tmp = strtok(buffer, ARGS_SEPARATOR);
        if(tmp != NULL && *tmp != '\n' && *tmp != '\t')
            lines_number++;
    }
    
    fclose(input_file);
    return lines_number;
}

char** parse_input_file(char* input_file_name, int* commands_number)
{
    *commands_number = count_lines(input_file_name);
    if(*commands_number == -1)
        return NULL;

    char** commands = (char**) malloc(*commands_number * sizeof(char*));
    if(commands == NULL)
    {
        printf("Error with allocating array for commands!\n");
        return NULL;
    }
    
    FILE* input_file = fopen(input_file_name, "r");
    if(input_file == NULL)
    {
        printf("Error with opening input file!\n");
        return NULL;
    }

    char buffer[MAX_LINE_LENGTH];

    for(int i = 0; i < *commands_number; i++)
    {
        fgets(buffer, MAX_LINE_LENGTH, input_file);
        commands[i] = (char*) malloc(strlen(buffer) + 1); // 1 for null byte
        strcpy(commands[i], buffer);
        char* tmp = strtok(buffer, ARGS_SEPARATOR);
        while(tmp == NULL || *tmp == '\n' || *tmp == '\t')
        {
            free(commands[i]);
            fgets(buffer, MAX_LINE_LENGTH, input_file);
            commands[i] = (char*) malloc(strlen(buffer) + 1); // 1 for null byte
            strcpy(commands[i], buffer);
            tmp = strtok(buffer, ARGS_SEPARATOR);
        }
    }

    fclose(input_file);
    return commands;
}

char** parse_command(char* command)
{
    int i = 0;
    char** result = NULL;
    char delimiters[NUM_OF_DELIMITERS] = DELIMITERS;

    char* buffer = strtok(command, delimiters);
    while(buffer != NULL)
    {
        i++;
        result = (char**) realloc(result, sizeof(char*) * (i + 1));
        if(result == NULL)
        {
            printf("Error while parsing command: %s\n", command);
            return NULL;
        }

        result[i - 1] = malloc((strlen(buffer) + 1) * sizeof(char)); // +1 for null byte
        strcpy(result[i - 1], buffer);
        buffer = strtok(NULL, delimiters);
    }

    result[i] = NULL; // to have arguments number in correct form for exec function

    return result;
}

int calculate_piped_commands(char* command)
{
    if(command == NULL)
    {
        printf("Uncorrect command to calculate piped elemets given!\n");
        return -1;
    }

    int result = 1; // when there are no pipes, there is still one command in given string
    int j = 0;
    char c = command[j];
    
    while(c != '\0')
    {
        if(c == '|')
            result++;
        j++;
        c = command[j];
    }

    return result;
}

int execute_pipeline(char* command)
{
    printf("Executing command: %s\n", command);
    int contained_commands = calculate_piped_commands(command);
    char* current_command = strtok(command, PIPE_SEPARATOR);
    int pipes[contained_commands - 1][PIPE_ARRAY_SIZE];
    char* commands[contained_commands];

    for(int i = 0; i < contained_commands; i++)
    {
        commands[i] = malloc((strlen(current_command) + 1) * sizeof(char)); // 1 for null byte
        strcpy(commands[i], current_command);
        current_command = strtok(NULL, PIPE_SEPARATOR);
    }    

    for(int i = 0; i < contained_commands; i++)
    {
        if(i < contained_commands - 1)
        {
            if(pipe(pipes[i]) == -1)
            {
                printf("Error with creating pipe!\n");
                exit(1);
            }
        }

        for(int j = 0; j < i - 1; j++)
        {
            close(pipes[j][PIPE_READ_INDEX]);
            close(pipes[j][PIPE_WRITE_INDEX]);
        }

        char** cmd_arguments = parse_command(commands[i]);
        
        pid_t child_pid = fork();
        if(child_pid == 0)
        {
            if(i > 0)
            {   
                close(pipes[i - 1][PIPE_WRITE_INDEX]);             
                if(dup2(pipes[i - 1][PIPE_READ_INDEX], STDIN_FILENO) == -1)
                {
                    printf("Error with casting dup2 for reading!\n");
                    exit(1);
                } 
                close(pipes[i - 1][PIPE_READ_INDEX]);               
            }

            if(i < contained_commands - 1)
            {
                close(pipes[i][PIPE_READ_INDEX]);
                if(dup2(pipes[i][PIPE_WRITE_INDEX], STDOUT_FILENO) == -1)
                {
                    printf("Error with casting dup2 for writing!\n");
                    exit(1);
                }
                close(pipes[i][PIPE_WRITE_INDEX]);              
            }
            execvp(cmd_arguments[0], cmd_arguments);
            perror("Error with performing execvp!");
            exit(1);
        }                           
    }

    for(int i = 0; i < contained_commands - 1; i++)
    {
        close(pipes[i][PIPE_READ_INDEX]);
        close(pipes[i][PIPE_WRITE_INDEX]);
        wait(NULL);
    }
    
    wait(NULL);
    printf("\n");
    
    return 0;
}

int execute_commands(char** commands, int commands_number)
{
    for(int i = 0; i < commands_number; i++)
    {
        pid_t child_process = fork();
        if(child_process == 0)
        {
            execute_pipeline(commands[i]);
            exit(0);
        }
        else
        {
           wait(NULL);
        }       
    }

    return 0;
}

int main(int argc, char* argv[])
{
    if(argc != CORRECT_ARGC_NUMBER)
    {
        printf("Uncorrect number of initial arguments!\n");
        return 1;
    }

    char* input_file_name = argv[FILE_NAME_ARGUMENT_POSITION];
    int commands_number = 0;
    char** commands = parse_input_file(input_file_name, &commands_number);
    if(commands == NULL || commands_number == -1)
        return 1;
    printf("Commands number: %d\n\n", commands_number);
    
    if(execute_commands(commands, commands_number) != 0)
        return 1;

    return 0;
}