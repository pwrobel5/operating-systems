#include "cw01_lib.h"
#include <stdio.h>

int main()
{
    printf("\n\n***********TESTING CW01 LIBRARY**********\n\n");

    printf("1) Creating new pointer array with size 10...\n");
    struct pointer_array* ptr_arr = create_pointer_array(10);
    printf("successfully created a pointer array, initial attributes:\n");
    printf("\tarr_size: %d, last_ptr: %d, find_cmd: %s\n", ptr_arr->array_size, ptr_arr->last_pointer, ptr_arr->find_command);
    printf("\n");

    printf("2) Executing find function with directory /usr/local/, file name intellij* and find_out.txt as output file name...\n");
    execute_find(ptr_arr, "/usr/local/", "intellij*", "find_out.txt");
    printf("successfull execution\n\n");

    printf("3) Creating 25 blocks of data read from find from previous step...\n");
    for(int i = 0; i <= 25; i++){
        printf("\tsuccessful for block number: %d\n", read_find_block(ptr_arr));
    }
    printf("\tnew pointer array attributes:\n");
    printf("\t\tarr_size: %d, last_ptr: %d, find_cmd: %s\n", ptr_arr->array_size, ptr_arr->last_pointer, ptr_arr->find_command);
    printf("\n");

    printf("\texample block number 23 contains:\n");
    int i = 0;
    int block = 23;
    char character = ptr_arr->arr[block][i];
    printf("\t\t");
    while(character != '\0')
    {
        while(character != '\n')
        {
            printf("%c", character);
            i++;
            character = ptr_arr->arr[block][i];
        }
        printf("%c", character);
        i++;
        character = ptr_arr->arr[block][i];
        printf("\t\t");
    }
    printf("\n\n");

    printf("4) Deleting blocks from table...\n");
    printf("\tdeleting block number 2\n");
    delete_block(ptr_arr, 2);
    printf("\t\tsuccessful\n");

    printf("\ttrying to delete block with index not present in table (200)\n");
    delete_block(ptr_arr, 200);
    printf("\t\tsuccessful\n");

    printf("\ttrying to delete deleted block number 2\n");
    delete_block(ptr_arr, 2);
    printf("\t\tsuccessful\n");

    printf("\n\n***********TESTING CW01 LIBRARY ENDED**********\n\n");

    return 0;
}
