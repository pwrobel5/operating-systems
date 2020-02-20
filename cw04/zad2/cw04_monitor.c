#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <unistd.h>
#include <dirent.h>

#define MINIMUM_NUMBER_OF_ARGUMENTS 2
#define LIST_FILE_NAME_POSITION 1

#define MAX_LINE_LENGTH 100
#define SEPARATOR " "

#define MAX_FILE_MEMORY_SIZE 1024

#define DATE_LENGTH 21
#define DATE_FORMAT "_%Y-%m-%d_%H-%M-%S"
#define ARCHIVE_PATH "./archive/"
#define ARCHIVE_PATH_LENGTH 10

#define SCANF_BUFFER_LENGTH 15
#define SEPARATOR " "
#define LIST_COMMAND "LIST"
#define STOP_COMMAND "STOP"
#define START_COMMAND "START"
#define ALL_FLAG "ALL"
#define END_COMMAND "END"

enum mode {memory, cp};

struct process_info {
    pid_t pid;
    char* filename;
};

volatile int copies_number = 0;
volatile int is_child_running = 1;
struct process_info* running_processes = NULL;
int files_number = 0;

void get_sigusr1_child(int signum)
{
    is_child_running = 0;
}

void get_sigusr2_child(int signum)
{
    is_child_running = 1;
}

void get_sigint_child(int signum)
{
    exit(copies_number);
}

char* get_full_file_path(char* file_name, char* file_path)
{
    char* result = (char*) malloc((strlen(file_path) + strlen(file_name) + 1) * sizeof(char)); // 1 for nullbyte
    strcpy(result, file_path);
    strcat(result, file_name);

    return result;
}

char* get_copy_file_path(char* file_name, time_t modification_date)
{
    char* result = (char*) malloc((ARCHIVE_PATH_LENGTH + strlen(file_name) + DATE_LENGTH + 1) * sizeof(char)); // 1 for nullbyte
    
    strcpy(result, ARCHIVE_PATH);
    strcat(result, file_name);
    
    char* date_string = (char*) malloc((DATE_LENGTH + 1) * sizeof(char)); // 1 for null byte
    strftime(date_string, DATE_LENGTH, DATE_FORMAT, localtime(&modification_date));
    strcat(result, date_string);

    free(date_string);

    return result;
}

void load_file_to_memory(char* full_file_path, char** buffer, int* file_length)
{
    FILE* monitored_file = fopen(full_file_path, "r");
    if(monitored_file == NULL)
    {
        printf("File not opened!\n");
        return;
    }
    
    int i = 0;
    char c;

    for(c = getc(monitored_file); c != EOF && i < MAX_FILE_MEMORY_SIZE; c = getc(monitored_file))
    {
        (*buffer)[i] = c;
        i++;
    }
    
    *file_length = i;
    fclose(monitored_file);
}

void make_copy_file(char* name_of_copy, char* buffer, int file_length)
{
    FILE* copied_file = fopen(name_of_copy, "w");
    if(copied_file == NULL)
    {
        printf("NULL for copy!\n");
        return;
    }        
    
    for(int i = 0; i < file_length; i++)
    {
        fputc(buffer[i], copied_file);
    }

    fclose(copied_file);
}

void monitor_file_memory(char* file_name, char* file_path, int monitoring_frequency)
{
    int file_length = 0;

    char* full_file_path = get_full_file_path(file_name, file_path);
    time_t modification_date = 0;
    struct stat* monitored_file_stat = (struct stat*) malloc(sizeof(struct stat));

    char* buffer = (char*) malloc(MAX_FILE_MEMORY_SIZE * sizeof(char));
    if(buffer == NULL)
    {
        printf("NULL buffer!\n");
        return;
    }

    load_file_to_memory(full_file_path, &buffer, &file_length);

    while(1)
    {
        if(is_child_running == 0)
        {
            pause();
        }

        lstat(full_file_path, monitored_file_stat);
        if(modification_date < monitored_file_stat->st_mtime)
        {
            modification_date = monitored_file_stat->st_mtime;

            char* copy_file_name = get_copy_file_path(file_name, modification_date);
            make_copy_file(copy_file_name, buffer, file_length);
            copies_number++;
            free(copy_file_name);

            load_file_to_memory(full_file_path, &buffer, &file_length);
        }
    }

    free(buffer);
    free(monitored_file_stat);
    free(full_file_path);
}

void print_process_info()
{
    printf("List of processes ran by program:\n");

    for(int i = 0; i < files_number; i++)
    {
        printf("Process PID: %d for file %s\n", (int) running_processes[i].pid, running_processes[i].filename);
    }
    printf("\n");
}

void end_all_processes()
{
    printf("Ending all running child processes...\n");

    int* status = (int*) malloc(sizeof(int));
    if(status == NULL)
    {
        printf("Error with allocating variable for status!\n");
        return;
    }

    for(int i = 0; i < files_number; i++)
    {
        kill(running_processes[i].pid, SIGINT);

        waitpid(running_processes[i].pid, status, 0);
        if(WIFEXITED(*status) == 1)
        {
            printf("Proces %d made %d copies of file %s\n", (int) running_processes[i].pid, WEXITSTATUS(*status), running_processes[i].filename);
        }
        else
        {
            printf("Error - process did not have exit value\n");
        }        
    }

    exit(0);
}

void stop_all_processes()
{
    printf("Stopping all running processes...\n");

    for(int i = 0; i < files_number; i++)
    {
        kill(running_processes[i].pid, SIGUSR1);
    }
}

void stop_one_process(int pid)
{
    printf("Stopping process of PID %d\n", pid);

    int pid_found = 0;

    for(int i = 0; i < files_number && pid_found == 0; i++)
    {
        if((int) running_processes[i].pid == pid)
            pid_found = 1;
    }

    if(pid_found != 1)
    {
        printf("There is no process with given PID!\n");
        return;
    }
    else
    {
        kill(pid, SIGUSR1);
    }    
}

void start_all_processes()
{
    printf("Starting all processes...\n");

    for(int i = 0; i < files_number; i++)
    {
        kill(running_processes[i].pid, SIGUSR2);
    }
}

void start_one_process(int pid)
{
    printf("Starting process of PID %d\n", pid);

    int pid_found = 0;

    for(int i = 0; i < files_number && pid_found == 0; i++)
    {
        if((int) running_processes[i].pid == pid)
            pid_found = 1;
    }

    if(pid_found != 1)
    {
        printf("There is no process with given PID!\n");
        return;
    }
    else
    {
        kill(pid, SIGUSR2);
    }    
}

void get_sigint_mother_process(int signum)
{
    end_all_processes();
    exit(0);    
}

void monitor_files(char** file_names, char** file_paths, int* monitoring_frequencies)
{
    running_processes = (struct process_info*) malloc(files_number * sizeof(struct process_info));

    for(int i = 0; i < files_number; i++)
    {
        pid_t child_pid = fork();
        if(child_pid == 0)
        {
            struct sigaction sigusr1_child;
            sigusr1_child.sa_handler = get_sigusr1_child;
            sigemptyset(&sigusr1_child.sa_mask);
            sigusr1_child.sa_flags = 0;
            sigaction(SIGUSR1, &sigusr1_child, NULL);

            struct sigaction sigusr2_child;
            sigusr2_child.sa_handler = get_sigusr2_child;
            sigemptyset(&sigusr2_child.sa_mask);
            sigusr2_child.sa_flags = 0;
            sigaction(SIGUSR2, &sigusr2_child, NULL);

            struct sigaction sigint_child;
            sigint_child.sa_handler = get_sigint_child;
            sigemptyset(&sigint_child.sa_mask);
            sigint_child.sa_flags = 0;
            sigaction(SIGINT, &sigint_child, NULL);

            monitor_file_memory(file_names[i], file_paths[i], monitoring_frequencies[i]);
        }
        else {
            running_processes[i].pid = child_pid;
            running_processes[i].filename = file_names[i];
        }
    }

    print_process_info();

    char* first_argument = (char*) malloc(SCANF_BUFFER_LENGTH * sizeof(char));
    char* second_argument = (char*) malloc(SCANF_BUFFER_LENGTH * sizeof(char));

    while(1)
    {
        printf("Type command: ");
        scanf("%s", first_argument);

        if(strcmp(first_argument, LIST_COMMAND) == 0)
        {
            print_process_info();
        }
        else if(strcmp(first_argument, END_COMMAND) == 0)
        {
            end_all_processes();
        }
        else {
            scanf("%s", second_argument);
            int given_pid = atoi(second_argument);
            
            if(strcmp(first_argument, STOP_COMMAND) == 0)
            {
                if(strcmp(second_argument, ALL_FLAG) == 0)
                {
                    stop_all_processes();
                }
                else if(given_pid > 0)
                {
                    stop_one_process(given_pid);
                }
                else
                {
                    printf("Wrong second argument given!\n");
                }                
            }
            else if(strcmp(first_argument, START_COMMAND) == 0)
            {
                if(strcmp(second_argument, ALL_FLAG) == 0)
                {
                    start_all_processes();
                }
                else if(given_pid > 0)
                {
                    start_one_process(given_pid);
                }
                else
                {
                    printf("Wrong second argument given!\n");
                }
            }
            else
            {
                printf("Wrong command!\n");
            }            
        }
    }
}

int get_file_lines_number(FILE* input_file)
{
    char c;
    int lines_number = 0;

    for(c = getc(input_file); c != EOF; c = getc(input_file))
    {
        if(c == '\n')
            lines_number++;
    }

    // return to the beginning of file
    fseek(input_file, 0, 0);
    return lines_number;
}

int read_from_file_list(char* list_file_name, char*** file_names, char*** file_paths, int** monitoring_frequencies)
{
    FILE* list_file = fopen(list_file_name, "r");

    if(list_file == NULL)
    {
        printf("Error with opening list file!\n");
        return 1;
    }

    char* buffer = (char*) malloc(MAX_LINE_LENGTH * sizeof(char));
    char* tmp = NULL;
    files_number = get_file_lines_number(list_file);
    *file_names = (char**) malloc(files_number * sizeof(char*));
    *file_paths = (char**) malloc(files_number * sizeof(char*));
    *monitoring_frequencies = (int*) malloc(files_number * sizeof(int));
    int i = 0;

    while(fgets(buffer, MAX_LINE_LENGTH, list_file) != NULL)
    {
        tmp = strtok(buffer, SEPARATOR);
        if(tmp == NULL || *tmp == '\n') 
        {
            printf("WARNING: blank line found, stop reading\n");
            files_number = i;
            *file_names = (char**) realloc(*file_names, files_number * sizeof(char*));
            *file_paths = (char**) realloc(*file_paths, files_number * sizeof(char*));
            return 0;
        }
        (*file_names)[i] = (char*) malloc((strlen(tmp) + 1) * sizeof(char));
        strcpy((*file_names)[i], tmp);

        tmp = strtok(NULL, SEPARATOR);
        if(tmp == NULL || *tmp == '\n') 
        {
            printf("Incorrect line found!\n");
            files_number = i;
            return 1;
        }
        (*file_paths)[i] = (char*) malloc((strlen(tmp) + 1) * sizeof(char));
        strcpy((*file_paths)[i], tmp);

        (*monitoring_frequencies)[i] = atoi(strtok(NULL, SEPARATOR));
        if((*monitoring_frequencies)[i] == 0)
        {
            printf("WARNING: Incorrect monitoring frequency or 0 value written in line %d\n", i + 1); // i starts from 0 while row numbers inf file start from 2s
        }
        i++;
    }

    fclose(list_file);
    free(buffer);

    return 0;
}

int parse_initial_arguments(int argc, char* argv[], char** list_file_name)
{
    if(argc < MINIMUM_NUMBER_OF_ARGUMENTS)
    {
        printf("Too few arguments given!\n");
        return 1;
    }

    *list_file_name = argv[LIST_FILE_NAME_POSITION];

    return 0;
}

void check_archive_directory(void)
{
    DIR* archive_directory = opendir(ARCHIVE_PATH);
    if(archive_directory == NULL)
    {
        printf("Archive directory does not exist, creating one\n");
        if(mkdir(ARCHIVE_PATH, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
            printf("Error with directory\n");
    }
    else 
    {
        closedir(archive_directory);
    }
}

int main(int argc, char* argv[])
{
    struct sigaction sigint_mother;
    sigint_mother.sa_handler = get_sigint_mother_process;
    sigemptyset(&sigint_mother.sa_mask);
    sigint_mother.sa_flags = 0;
    sigaction(SIGINT, &sigint_mother, NULL);

    char* list_file_name;

    if(parse_initial_arguments(argc, argv, &list_file_name) != 0)
        return 1;

    char** file_names = NULL;
    char** file_paths = NULL;
    int* monitoring_frequencies = NULL;

    if(read_from_file_list(list_file_name, &file_names, &file_paths, &monitoring_frequencies) != 0)
    {
        return 1;
    }
    check_archive_directory();
    
    monitor_files(file_names, file_paths, monitoring_frequencies);

    return 0;
}