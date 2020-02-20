#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CORRECT_NUMBER_OF_ARGUMENTS 5
#define FILE_NAME_POSITION 1
#define PMIN_POSITION 2
#define PMAX_POSITION 3
#define BYTES_POSITION 4

#define DATE_FORMAT "%d-%m-%Y %H:%M:%S"
#define DATE_LENGTH 20

int main(int argc, char* argv[])
{
    if(argc != CORRECT_NUMBER_OF_ARGUMENTS)
        return 1;
    
    char* filename = argv[FILE_NAME_POSITION];
    int pmin = atoi(argv[PMIN_POSITION]);
    int pmax = atoi(argv[PMAX_POSITION]);
    int bytes = atoi(argv[BYTES_POSITION]);

    if(pmin >= pmax)
    {
        printf("Pmin bigger or equal pmax!\n");
        return 1;
    }

    FILE* output_file;
    char byte_to_write = 'A';
    char date_string[DATE_LENGTH];
    int modulo_factor = pmax - pmin + 1;
    int random_int = 0;
    int pid = 0;
    srand(time(NULL));

    while(1)
    {
        random_int = (rand() % modulo_factor) + pmin;
        sleep(random_int);

        time_t current_time = time(NULL);
        strftime(date_string, DATE_LENGTH, DATE_FORMAT, localtime(&current_time));
        pid = (int) getpid();

        output_file = fopen(filename, "a");
        if(output_file == NULL)
        {
            printf("Error with opening file!\n");
            return 1;
        }
        
        fprintf(output_file, "%d %d %s ", pid, random_int, date_string);
        for(int i = 0; i < bytes; i++)
        {
            fprintf(output_file, "%c", byte_to_write);
        }
        fprintf(output_file, "\n");
        fclose(output_file);
    }

    return 0;
}