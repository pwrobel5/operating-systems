#include "../zad1/cw01_lib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

#define DEFAULT_POINTER_ARRAY_SIZE 10
#define CREATE_TABLE_COMMAND "create_table"
#define SEARCH_DIRECTORY_COMMAND "search_directory"
#define SEARCH_DIRECTORY_COMMAND_ATTRIBUTES 3
#define REMOVE_BLOCK_COMMAND "remove_block"

double calculate_time_difference_in_ms(clock_t start_point, clock_t end_point)
{
    return (double) 1000 * (end_point - start_point) / (1.0 * sysconf(_SC_CLK_TCK)); //1000 to get value in ms
}

void write_times_to_file(FILE* output_file, clock_t real_start, struct tms* tms_start, clock_t real_end, struct tms* tms_end)
{
    fprintf(output_file, "\tReal time: \t\t\t%f ms\n", calculate_time_difference_in_ms(real_start, real_end));
    fprintf(output_file, "\tUser mode time: \t\t%f ms\n", calculate_time_difference_in_ms(tms_start->tms_utime, tms_end->tms_utime));
    fprintf(output_file, "\tKernel mode time: \t\t%f ms\n", calculate_time_difference_in_ms(tms_start->tms_stime, tms_end->tms_stime));
    fprintf(output_file, "\n\n");
}

int get_file_size(char* file_name)
{
    struct stat* st = (struct stat*) malloc(sizeof(struct stat));
    stat(file_name, st);
    return st->st_size;
}

struct pointer_array* parse_initial_arguments_dynamically(int argc, char* argv[])
{
    void *handle = dlopen("../zad1/libshcw01_lib.so", RTLD_LAZY);
    if(handle == NULL)
    {
        printf("Error in loading library!\n");
        return NULL;
    }

    int i = 1;
    char* current_argument;
    struct pointer_array* result = NULL;

    char* output_file_name = argv[i];
    i++;

    FILE* output_file = fopen(output_file_name, "w");
    if(output_file == NULL)
    {
        printf("Error opening output file!\n");
        return NULL;
    }

    clock_t start_point = 0;
    struct tms* tms_start_point = (struct tms*) malloc(sizeof(struct tms));
    clock_t end_point = 0;
    struct tms* tms_end_point = (struct tms*) malloc(sizeof(struct tms));

    while(i < argc)
    {
        current_argument = argv[i];
        i++;

        if(strcmp(current_argument, CREATE_TABLE_COMMAND) == 0)
        {
            if(i < argc)
            {
                int table_size = atoi(argv[i]);
                fprintf(output_file, "Creation of pointer array of size ");
                struct pointer_array* (*create_array)(int) = (struct pointer_array* (*) (int)) dlsym(handle, "create_pointer_array");

                if(table_size == 0)
                {
                    printf("Invalid or no initial table size given, using default value of 10\n");
                    printf("Executing command: %s %d\n", CREATE_TABLE_COMMAND, DEFAULT_POINTER_ARRAY_SIZE);
                    fprintf(output_file, "%d:\n", DEFAULT_POINTER_ARRAY_SIZE);

                    start_point = times(tms_start_point);
                    result = (*create_array)(DEFAULT_POINTER_ARRAY_SIZE);
                    end_point = times(tms_end_point);
                    write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);
                }
                else
                {
                    fprintf(output_file, "%d:\n", table_size);
                    printf("Executing command: %s %d\n", CREATE_TABLE_COMMAND, table_size);
                    start_point = times(tms_start_point);
                    result = (*create_array)(table_size);
                    end_point = times(tms_end_point);
                    write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                    i++;
                }
            }
            else
            {
                printf("Too few arguments, creating table with default size 10\n");
                result = create_pointer_array(DEFAULT_POINTER_ARRAY_SIZE);
            }
        }
        else if(strcmp(current_argument, SEARCH_DIRECTORY_COMMAND) == 0)
        {
            if(argc - i < SEARCH_DIRECTORY_COMMAND_ATTRIBUTES)
            {
                printf("Too few arguments for searching directory, operation skipped\n");
            }
            else if(result == NULL)
            {
                printf("Result pointer table not initialized! Skipping searching\n");
            }
            else
            {
                void (*execute_f)(struct pointer_array*, char*, char*, char*) = (void (*) (struct pointer_array*, char*, char*, char*)) dlsym(handle, "execute_find");
                fprintf(output_file, "Executing find command for directory ");
                char* directory_name;
                char* file_name;
                char* tmp_file_name;

                directory_name = argv[i];
                i++;
                fprintf(output_file, "%s, file ", directory_name);

                file_name = argv[i];
                i++;
                fprintf(output_file, "%s:\n", file_name);

                tmp_file_name = argv[i];
                i++;

                printf("Executing command: %s %s %s %s\n", SEARCH_DIRECTORY_COMMAND, directory_name, file_name, tmp_file_name);

                start_point = times(tms_start_point);
                (*execute_f)(result, directory_name, file_name, tmp_file_name);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                fprintf(output_file, "Adding to array block of size %d:\n", get_file_size(tmp_file_name));

                start_point = times(tms_start_point);
                int (*read_block)(struct pointer_array*) = (int (*) (struct pointer_array*)) dlsym(handle, "read_find_block");
                (*read_block)(result);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);
            }
        }
        else if(strcmp(current_argument, REMOVE_BLOCK_COMMAND) == 0)
        {
            if(result == NULL)
            {
                printf("Result pointer table not initialized! Skipping removing\n");
            }
            if(i < argc)
            {
                void (*remove_block)(struct pointer_array*, int) = (void (*) (struct pointer_array*, int)) dlsym(handle, "delete_block");

                int index = atoi(argv[i]);
                fprintf(output_file, "Removing block of index %d:\n", index);
                printf("Executing command: %s %d\n", REMOVE_BLOCK_COMMAND, index);

                start_point = times(tms_start_point);
                (*remove_block)(result, index);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                i++;
            }
            else
            {
                printf("Not given index of element to remove!\n");
            }
        }
        else
        {
            printf("Command not recognized: %s\n", current_argument);
        }
    }

    fclose(output_file);
    dlclose(handle);
    return result;
}

struct pointer_array* parse_initial_arguments(int argc, char* argv[])
{
    int i = 1;
    char* current_argument;
    struct pointer_array* result = NULL;

    char* output_file_name = argv[i];
    i++;

    FILE* output_file = fopen(output_file_name, "w");
    if(output_file == NULL)
    {
        printf("Error opening output file!\n");
        return NULL;
    }

    clock_t start_point = 0;
    struct tms* tms_start_point = (struct tms*) malloc(sizeof(struct tms));
    clock_t end_point = 0;
    struct tms* tms_end_point = (struct tms*) malloc(sizeof(struct tms));

    while(i < argc)
    {
        current_argument = argv[i];
        i++;

        if(strcmp(current_argument, CREATE_TABLE_COMMAND) == 0)
        {
            if(i < argc)
            {
                int table_size = atoi(argv[i]);
                fprintf(output_file, "Creation of pointer array of size ");

                if(table_size == 0)
                {
                    printf("Invalid or no initial table size given, using default value of 10\n");
                    printf("Executing command: %s %d\n", CREATE_TABLE_COMMAND, DEFAULT_POINTER_ARRAY_SIZE);
                    fprintf(output_file, "%d:\n", DEFAULT_POINTER_ARRAY_SIZE);

                    start_point = times(tms_start_point);
                    result = create_pointer_array(DEFAULT_POINTER_ARRAY_SIZE);
                    end_point = times(tms_end_point);
                    write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);
                }
                else
                {
                    fprintf(output_file, "%d:\n", table_size);
                    printf("Executing command: %s %d\n", CREATE_TABLE_COMMAND, table_size);
                    start_point = times(tms_start_point);
                    result = create_pointer_array(table_size);
                    end_point = times(tms_end_point);
                    write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                    i++;
                }
            }
            else
            {
                printf("Too few arguments, creating table with default size 10\n");
                result = create_pointer_array(DEFAULT_POINTER_ARRAY_SIZE);
            }
        }
        else if(strcmp(current_argument, SEARCH_DIRECTORY_COMMAND) == 0)
        {
            if(argc - i < SEARCH_DIRECTORY_COMMAND_ATTRIBUTES)
            {
                printf("Too few arguments for searching directory, operation skipped\n");
            }
            else if(result == NULL)
            {
                printf("Result pointer table not initialized! Skipping searching\n");
            }
            else
            {
                fprintf(output_file, "Executing find command for directory ");
                char* directory_name;
                char* file_name;
                char* tmp_file_name;

                directory_name = argv[i];
                i++;
                fprintf(output_file, "%s, file ", directory_name);

                file_name = argv[i];
                i++;
                fprintf(output_file, "%s:\n", file_name);

                tmp_file_name = argv[i];
                i++;

                printf("Executing command: %s %s %s %s\n", SEARCH_DIRECTORY_COMMAND, directory_name, file_name, tmp_file_name);

                start_point = times(tms_start_point);
                execute_find(result, directory_name, file_name, tmp_file_name);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                fprintf(output_file, "Adding to array block of size %d:\n", get_file_size(tmp_file_name));

                start_point = times(tms_start_point);
                read_find_block(result);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);
            }
        }
        else if(strcmp(current_argument, REMOVE_BLOCK_COMMAND) == 0)
        {
            if(result == NULL)
            {
                printf("Result pointer table not initialized! Skipping removing\n");
            }
            if(i < argc)
            {
                int index = atoi(argv[i]);
                fprintf(output_file, "Removing block of index %d:\n", index);
                printf("Executing command: %s %d\n", REMOVE_BLOCK_COMMAND, index);

                start_point = times(tms_start_point);
                delete_block(result, index);
                end_point = times(tms_end_point);
                write_times_to_file(output_file, start_point, tms_start_point, end_point, tms_end_point);

                i++;
            }
            else
            {
                printf("Not given index of element to remove!\n");
            }
        }
        else
        {
            printf("Command not recognized: %s\n", current_argument);
        }
    }

    fclose(output_file);
    return result;
}

int main(int argc, char* argv[])
{
    printf("\n\n***********TESTING CW01 LIBRARY**********\n\n");

    if(argc == 1)
    {
        printf("No arguments passed!\n");
        return 1;
    }

    struct pointer_array* ptr_arr;

    if(strcmp(argv[0], "cw01_program_dynamic.x") == 0)
    {
        ptr_arr = parse_initial_arguments_dynamically(argc, argv);
    }
    else
    {
        ptr_arr = parse_initial_arguments(argc, argv);
    }

    if(ptr_arr == NULL)
    {
        printf("Failed in creating pointer array!\n");
        return 2;
    }

    printf("Pointer array attributes:\n");
    printf("\tarr_size: %d, last_ptr: %d, find_cmd: %s\n", ptr_arr->array_size, ptr_arr->last_pointer, ptr_arr->find_command);

    printf("\n\n***********TESTING CW01 LIBRARY ENDED**********\n\n");

    return 0;
}
