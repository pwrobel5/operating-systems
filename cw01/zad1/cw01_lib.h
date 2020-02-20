#ifndef CW01_LIB_H
#define CW01_LIB_H

struct pointer_array {
    int array_size;
    int last_pointer;
    char** arr;
    char* find_command;
    char* output_file_name;
};

struct pointer_array* create_pointer_array(int initial_size);

void execute_find(struct pointer_array* ptr_arr, char* directory_name, char* file_name, char* output_file_name);

int read_find_block(struct pointer_array* ptr_arr);

void delete_block(struct pointer_array* ptr_arr, int block_index);

#endif
