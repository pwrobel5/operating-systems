#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#include <dirent.h>

#define MINIMUM_NUMBER_OF_ARGUMENTS 6
#define LIST_FILE_NAME_POSITION 1
#define MONITOR_MAX_TIME_POSITION 2
#define MODE_POSITION 3
#define PROCESSOR_MAX_TIME_POSITION 4
#define VIRTUAL_MAX_MEMORY_POSITION 5

#define MAX_LINE_LENGTH 100
#define SEPARATOR " "

#define MAX_FILE_MEMORY_SIZE 1024

#define DATE_LENGTH 21
#define DATE_FORMAT "_%Y-%m-%d_%H-%M-%S"
#define ARCHIVE_PATH "./archive/"
#define ARCHIVE_PATH_LENGTH 10

#define MEGABYTE_TO_BYTE_MULTIPLIER 1024 * 1024

enum mode {memory, cp};

void print_resources_usage(struct rusage rsg)
{
    printf("\tuser CPU time used: %lu.%lu\n", rsg.ru_utime.tv_sec, rsg.ru_utime.tv_usec);
    printf("\tsystem CPU time used: %lu.%lu\n\n", rsg.ru_stime.tv_sec, rsg.ru_stime.tv_usec);
}

double calculate_time_difference(clock_t start_point, clock_t end_point)
{
    return (double) (end_point - start_point) / (1.0 * sysconf(_SC_CLK_TCK));
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

int monitor_file_cp(char* file_name, char* file_path, int monitoring_frequency, int monitor_max_time)
{
    int copies_number = 0;
    double passed_seconds = 0;
    
    clock_t start_point = 0;
    clock_t end_point = 0;

    char* full_file_path = get_full_file_path(file_name, file_path);
    time_t modification_date = 0;
    struct stat* monitored_file_stat = (struct stat*) malloc(sizeof(struct stat));

    while(passed_seconds <= monitor_max_time)
    {
        start_point = times(NULL);
        lstat(full_file_path, monitored_file_stat);
        if(modification_date < monitored_file_stat->st_mtime)
        {
            modification_date = monitored_file_stat->st_mtime;
            char* copy_file_path = get_copy_file_path(file_name, modification_date);

            if(fork() == 0)
            {
                execlp("cp", "cp", full_file_path, copy_file_path, NULL);
            }
            
            copies_number++;
            free(copy_file_path);
        }
        end_point = times(NULL);

        double time_in_loop = calculate_time_difference(start_point, end_point);
        passed_seconds += time_in_loop;
        passed_seconds += monitoring_frequency;

        if(passed_seconds <= monitor_max_time)
        {
            sleep(monitoring_frequency);
        }
    }

    free(monitored_file_stat);
    free(full_file_path);
    return copies_number;
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

int monitor_file_memory(char* file_name, char* file_path, int monitoring_frequency, int monitor_max_time)
{
    int copies_number = 0;
    double passed_seconds = 0;
    int file_length = 0;

    char* full_file_path = get_full_file_path(file_name, file_path);
    time_t modification_date = 0;
    struct stat* monitored_file_stat = (struct stat*) malloc(sizeof(struct stat));

    char* buffer = (char*) malloc(MAX_FILE_MEMORY_SIZE * sizeof(char));
    if(buffer == NULL)
    {
        printf("NULL buffer!\n");
        return copies_number;
    }

    load_file_to_memory(full_file_path, &buffer, &file_length);

    clock_t start_point = 0;
    clock_t end_point = 0;

    while(passed_seconds <= monitor_max_time)
    {
        start_point = times(NULL);
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
        end_point = times(NULL);

        double time_in_loop = calculate_time_difference(start_point, end_point);
        passed_seconds += time_in_loop;
        passed_seconds += monitoring_frequency;

        if(passed_seconds <= monitor_max_time)
        {
            sleep(monitoring_frequency);
        }
    }

    free(buffer);
    free(monitored_file_stat);
    free(full_file_path);

    return copies_number;
}

void monitor_files(char** file_names, char** file_paths, int* monitoring_frequencies, int files_number, int monitor_max_time, enum mode* m, unsigned long processor_max_time, unsigned long virtual_max_memory)
{
    for(int i = 0; i < files_number; i++)
    {
        if(fork() == 0)
        {
            int copies_number = 0;

            struct rlimit* processor_time_limit = (struct rlimit*) malloc(sizeof(struct rlimit));
            processor_time_limit->rlim_max = (rlim_t) processor_max_time;
            processor_time_limit->rlim_cur = (rlim_t) processor_max_time;
            if(setrlimit(RLIMIT_CPU, processor_time_limit) != 0)
            {
                perror("Error setting CPU time limit for process!");
                exit(-1);
            }

            struct rlimit* virtual_memory_limit = (struct rlimit*) malloc(sizeof(struct rlimit));
            processor_time_limit->rlim_max = (rlim_t) virtual_max_memory;
            processor_time_limit->rlim_cur = (rlim_t) virtual_max_memory;
            if(setrlimit(RLIMIT_AS, virtual_memory_limit) != 0)
            {
                perror("Error setting max virtual memory for process!\n");
            }        

            switch (*m)
            {
                case memory:
                    copies_number = monitor_file_memory(file_names[i], file_paths[i], monitoring_frequencies[i], monitor_max_time);
                    break;
                case cp:
                    copies_number = monitor_file_cp(file_names[i], file_paths[i], monitoring_frequencies[i], monitor_max_time);
                    break;
                default:
                    break;
            }
            
            free(processor_time_limit);
            free(virtual_memory_limit);

            struct rusage single_process_rusage;
            getrusage(RUSAGE_SELF, &single_process_rusage);
            printf("Process %d resources usage:\n", (int) getpid());
            print_resources_usage(single_process_rusage);

            exit(copies_number);
        }
    }

    int* status = (int*) malloc(sizeof(int));
    if(status == NULL)
    {
        printf("Error with allocating variable for status!\n");
        return;
    }
    
    for(int i = 0; i < files_number; i++)
    {        
        pid_t child_pid = wait(status);
        if(WIFEXITED(*status) == 1)
        {
            printf("Proces %d utworzyl %d kopii pliku\n", (int) child_pid, WEXITSTATUS(*status));
        }
    }
    free(status);
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

int read_from_file_list(char* list_file_name, char*** file_names, char*** file_paths, int** monitoring_frequencies, int* lines_number)
{
    FILE* list_file = fopen(list_file_name, "r");

    if(list_file == NULL)
    {
        printf("Error with opening list file!\n");
        return 1;
    }

    char* buffer = (char*) malloc(MAX_LINE_LENGTH * sizeof(char));
    char* tmp = NULL;
    *lines_number = get_file_lines_number(list_file);
    *file_names = (char**) malloc(*lines_number * sizeof(char*));
    *file_paths = (char**) malloc(*lines_number * sizeof(char*));
    *monitoring_frequencies = (int*) malloc(*lines_number * sizeof(int));
    int i = 0;

    while(fgets(buffer, MAX_LINE_LENGTH, list_file) != NULL)
    {
        tmp = strtok(buffer, SEPARATOR);
        if(tmp == NULL || *tmp == '\n') 
        {
            printf("WARNING: blank line found, stop reading\n");
            *lines_number = i;
            *file_names = (char**) realloc(*file_names, *lines_number * sizeof(char*));
            *file_paths = (char**) realloc(*file_paths, *lines_number * sizeof(char*));
            return 0;
        }
        (*file_names)[i] = (char*) malloc((strlen(tmp) + 1) * sizeof(char));
        strcpy((*file_names)[i], tmp);

        tmp = strtok(NULL, SEPARATOR);
        if(tmp == NULL || *tmp == '\n') 
        {
            printf("Incorrect line found!\n");
            *lines_number = i;
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

int parse_initial_arguments(int argc, char* argv[], char** list_file_name, int* monitor_max_time, enum mode* m, unsigned long* processor_max_time, unsigned long* virtual_max_memory)
{
    if(argc < MINIMUM_NUMBER_OF_ARGUMENTS)
    {
        printf("Too few arguments given!\n");
        return 1;
    }

    *list_file_name = argv[LIST_FILE_NAME_POSITION];
    *monitor_max_time = atoi(argv[MONITOR_MAX_TIME_POSITION]);

    if(*monitor_max_time == 0)
    {
        printf("Wrong time given!\n");
        return 1;
    }
    
    if(strcmp(argv[MODE_POSITION], "memory_mode") == 0)
        *m = memory;
    else if(strcmp(argv[MODE_POSITION], "cp_mode") == 0)
        *m = cp;
    else
    {
        printf("Wrong mode given!\n");
        return 1;
    }    

    *processor_max_time = atol(argv[PROCESSOR_MAX_TIME_POSITION]);
    if(*processor_max_time == 0)
    {
        printf("Wrong processor max time given!\n");
        return 1;
    }

    *virtual_max_memory = atol(argv[VIRTUAL_MAX_MEMORY_POSITION]);
    if(*virtual_max_memory == 0)
    {
        printf("Wrong virtual memory limit given!\n");
    }

    return 0;
}

void free_arrays(char** file_names, char** file_paths, int* monitoring_frequencies, int lines_number, enum mode* m)
{
    for(int i = 0; i < lines_number; i++)
    {
        free(file_names[i]);
        free(file_paths[i]);
    }
    free(monitoring_frequencies);
    free(m);
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
    char* list_file_name;
    int monitor_max_time;
    enum mode* m = (enum mode*) malloc(sizeof(enum mode));
    unsigned long processor_max_time = 0;
    unsigned long virtual_max_memory = 0;

    if(parse_initial_arguments(argc, argv, &list_file_name, &monitor_max_time, m, &processor_max_time, &virtual_max_memory) != 0)
        return 1;

    // virtual_max_memory given in MB, setrlimit needs value in bytes
    virtual_max_memory = virtual_max_memory * MEGABYTE_TO_BYTE_MULTIPLIER; 

    char** file_names = NULL;
    char** file_paths = NULL;
    int* monitoring_frequencies = NULL;    
    int lines_number = 0;

    if(read_from_file_list(list_file_name, &file_names, &file_paths, &monitoring_frequencies, &lines_number) != 0)
    {
        free_arrays(file_names, file_paths, monitoring_frequencies, lines_number, m);
        return 1;
    }
    check_archive_directory();
    
    monitor_files(file_names, file_paths, monitoring_frequencies, lines_number, monitor_max_time, m, processor_max_time, virtual_max_memory);

    struct rusage all_children_usage;
    getrusage(RUSAGE_CHILDREN, &all_children_usage);

    printf("Child processes resource usage:\n");
    print_resources_usage(all_children_usage);

    free_arrays(file_names, file_paths, monitoring_frequencies, lines_number, m);
    return 0;
}