#include "cw01_lib.h"
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#define INITIAL_FIND_COMMAND_SIZE 6
#define INITIAL_FIND_COMMAND "find "
#define NAME_PARAMETER_SIZE 7
#define NAME_PARAMETER " -name "
#define STREAM_SIGN " > "
#define STREAM_SIGN_SIZE 3
#define POINTER_ARRAY_ENLARGE_FACTOR 2

struct pointer_array* create_pointer_array(int initial_size)
{
    struct pointer_array* result;

    result = (struct pointer_array*) malloc(sizeof(struct pointer_array));

    result->array_size = initial_size;
    result->last_pointer = -1; //created array is empty
    result->arr = (char**) calloc(initial_size, sizeof(char*));

    result->find_command = (char*) calloc(INITIAL_FIND_COMMAND_SIZE, sizeof(char));
    strcpy(result->find_command, INITIAL_FIND_COMMAND);

    result->output_file_name = NULL;

    return result;
}

void execute_find(struct pointer_array* ptr_arr, char* directory_name, char* file_name, char* output_file_name)
{
    int find_command_size = INITIAL_FIND_COMMAND_SIZE + strlen(directory_name) + NAME_PARAMETER_SIZE + strlen(file_name) + STREAM_SIGN_SIZE + strlen(output_file_name) + 1; // + 1 to have place for null byte
    free(ptr_arr->find_command);

    ptr_arr->find_command = (char*) malloc(find_command_size);

    strcpy(ptr_arr->find_command, INITIAL_FIND_COMMAND);
    strcat(ptr_arr->find_command, directory_name);
    strcat(ptr_arr->find_command, NAME_PARAMETER);
    strcat(ptr_arr->find_command, file_name);
    strcat(ptr_arr->find_command, STREAM_SIGN);
    strcat(ptr_arr->find_command, output_file_name);

    ptr_arr->output_file_name = output_file_name;

    system(ptr_arr->find_command);
}

void enlarge_pointer_array(struct pointer_array* ptr_arr)
{
    char** new_arr = (char**) realloc(ptr_arr->arr, ptr_arr->array_size * POINTER_ARRAY_ENLARGE_FACTOR * sizeof(char*));
    if (new_arr == NULL){
        printf("Array enlarging error!\n");
        return;
    }

    ptr_arr->arr = new_arr;
    ptr_arr->array_size *= POINTER_ARRAY_ENLARGE_FACTOR;
}

int read_find_block(struct pointer_array* ptr_arr)
{
    struct stat* st = (struct stat*) malloc(sizeof(struct stat));
    stat(ptr_arr->output_file_name, st);
    int temp_file_size = st->st_size;

    ptr_arr->last_pointer++;
    if(ptr_arr->last_pointer >= ptr_arr->array_size)
    {
        enlarge_pointer_array(ptr_arr);
    }

    ptr_arr->arr[ptr_arr->last_pointer] = (char*) calloc(temp_file_size, sizeof(char));
    if(ptr_arr->arr[ptr_arr->last_pointer] == NULL)
    {
        ptr_arr->last_pointer--;
        printf("Creating data block error!\n");
        return -1;
    }

    FILE* temp_file = fopen(ptr_arr->output_file_name, "r");
    fread(ptr_arr->arr[ptr_arr->last_pointer], temp_file_size, sizeof(char), temp_file);
    fclose(temp_file);

    return ptr_arr->last_pointer;
}

void delete_block(struct pointer_array* ptr_arr, int block_index)
{
    if(block_index >= ptr_arr->array_size)
    {
        return;
    }

    if(ptr_arr->arr[block_index] == NULL)
    {
        return;
    }

    free(ptr_arr->arr[block_index]);
    ptr_arr->arr[block_index] = NULL;
}
