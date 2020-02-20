#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <fcntl.h>
#include <unistd.h>

#define ARGC_FOR_GENERATE 5
#define ARGC_FOR_SORT 6
#define ARGC_FOR_COPY 7

#define COMMAND_POSITION 1

#define GENERATE_COMMAND "generate"
#define SORT_COMMAND "sort"
#define COPY_COMMAND "copy"

#define GENERATE_FILE_NAME_POSITION 2
#define GENERATE_RECORDS_POSITION 3
#define GENERATE_RECORDSIZE_POSITION 4

#define SORT_FILE_NAME_POSITION 2
#define SORT_RECORDS_POSITION 3
#define SORT_RECORDSIZE_POSITION 4
#define SORT_MODE_POSITION 5

#define COPY_SOURCE_NAME_POSITION 2
#define COPY_DESTINATION_NAME_POSITION 3
#define COPY_RECORDS_POSITION 4
#define COPY_RECORDSIZE_POSITION 5
#define COPY_MODE_POSITION 6

#define SYS_COMMAND "sys"
#define LIB_COMMAND "lib"

#define MAX_8BIT_NUMBER 255
#define SORT_COMPARED_INDEX 0

enum function {generate, sort, copy};
enum mode {sys, lib};

double calculate_time_difference_in_ms(clock_t start_point, clock_t end_point)
{
    return (double) 1000 * (end_point - start_point) / (1.0 * sysconf(_SC_CLK_TCK)); // 1000 to get value in ms
}

void print_times(clock_t real_start, struct tms* tms_start, clock_t real_end, struct tms* tms_end)
{
    printf("\tReal time: \t\t\t%f ms\n", calculate_time_difference_in_ms(real_start, real_end));
    printf("\tUser mode time: \t\t%f ms\n", calculate_time_difference_in_ms(tms_start->tms_utime, tms_end->tms_utime));
    printf("\tKernel mode time: \t\t%f ms\n", calculate_time_difference_in_ms(tms_start->tms_stime, tms_end->tms_stime));
}

int get_function_and_mode(int argc, char* argv[], enum function* f, enum mode* m)
{
    if(argc == ARGC_FOR_GENERATE && strcmp(argv[COMMAND_POSITION], GENERATE_COMMAND) == 0)
    {
        *f = generate;
    }
    else if(argc == ARGC_FOR_SORT && strcmp(argv[COMMAND_POSITION], SORT_COMMAND) == 0)
    {
        *f = sort;
        if(strcmp(argv[SORT_MODE_POSITION], SYS_COMMAND) == 0)
            *m = sys;
        else if(strcmp(argv[SORT_MODE_POSITION], LIB_COMMAND) == 0)
            *m = lib;
        else
        {
            errno = EINVAL;
            perror("Wrong sorting mode!");
            return 1;
        }
    }
    else if(argc == ARGC_FOR_COPY && strcmp(argv[COMMAND_POSITION], COPY_COMMAND) == 0)
    {
        *f = copy;
        if(strcmp(argv[COPY_MODE_POSITION], SYS_COMMAND) == 0)
            *m = sys;
        else if(strcmp(argv[COPY_MODE_POSITION], LIB_COMMAND) == 0)
            *m = lib;
        else
        {
            errno = EINVAL;
            perror("Wrong copying mode!");
            return 1;
        }
    }
    else
    {
        errno = EINVAL;
        perror("Incorrect command!");
        free(f);
        free(m);
        f = NULL;
        m = NULL;
        return 1;
    }

    return 0;
}

int check_generate_sort_parameters(char* filename, int records, int recordsize)
{
    if(filename == NULL)
    {
        errno = EINVAL;
        perror("Error with reading file name!");
        return 1;
    }

    if(records <= 0)
    {
        errno = EINVAL;
        perror("Wrong record number!");
        return 1;
    }

    if(recordsize <= 0)
    {
        errno = EINVAL;
        perror("Wrong record size!");
        return 1;
    }
    return 0;
}

void read_generate_parameters(char* argv[], char** filename, int* records, int* recordsize)
{
    *filename = argv[GENERATE_FILE_NAME_POSITION];
    *records = atoi(argv[GENERATE_RECORDS_POSITION]);
    *recordsize = atoi(argv[GENERATE_RECORDSIZE_POSITION]);
}

void read_sort_parameters(char* argv[], char** filename, int* records, int* recordsize)
{
    *filename = argv[SORT_FILE_NAME_POSITION];
    *records = atoi(argv[SORT_RECORDS_POSITION]);
    *recordsize = atoi(argv[SORT_RECORDSIZE_POSITION]);
}

int check_copy_parameters(char* source_name, char* destination_name, int records, int recordsize)
{
    if(source_name == NULL)
    {
        errno = EINVAL;
        perror("Error with reading source name!");
        return 1;
    }

    if(destination_name == NULL)
    {
        errno = EINVAL;
        perror("Error with reading destination name!");
        return 1;
    }

    if(records <= 0)
    {
        errno = EINVAL;
        perror("Wrong record number!");
        return 1;
    }

    if(recordsize <= 0)
    {
        errno = EINVAL;
        perror("Wrong record size!");
        return 1;
    }

    return 0;
}

void read_copy_parameters(char* argv[], char** source_name, char** destination_name, int* records, int* recordsize)
{
    *source_name = argv[COPY_SOURCE_NAME_POSITION];
    *destination_name = argv[COPY_DESTINATION_NAME_POSITION];
    *records = atoi(argv[COPY_RECORDS_POSITION]);
    *recordsize = atoi(argv[COPY_RECORDSIZE_POSITION]);
}

void print_file(char* filename, int records, int recordsize)
{
    int file_handle = open(filename, O_RDONLY);
    char* buff = (char*) malloc(records * sizeof(char));

    for(int i = 0; i < records; i++)
    {
        read(file_handle, buff, recordsize);

        unsigned char first = (unsigned char) buff[0];
        printf("first: %d record: ", first);

        for(int j = 0; j < recordsize; j++)
        {
            printf("%d ", buff[j]);
        }
        printf("\n");
    }

    close(file_handle);
}

void generate_record_file(char* filename, int records, int recordsize)
{
    int output = open(filename, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    int random_source = open("/dev/urandom", O_RDONLY);

    if(errno == EEXIST)
    {
        perror("File already exists!");
        errno = 0;
        close(output);
        return;
    }

    char* buff = (char*) malloc(recordsize * sizeof(char));
    srand(time(NULL));

    for(int i = 0; i < records; i++)
    {
        read(random_source, buff, recordsize);
        for(int j = 0; j < recordsize; j++)
        {
            buff[j] = buff[j] % MAX_8BIT_NUMBER;
        }
        write(output, buff, recordsize);
    }

    close(output);
    close(random_source);
}

int sort_file_sys(char* filename, int records, int recordsize)
{
    int sorted_file = open(filename, O_RDWR);

    if(sorted_file == -1)
    {
        perror("Error with opening file!");
        errno = 0;
        close(sorted_file);
        return 1;
    }

    unsigned char* smallest_record = (unsigned char*) malloc(recordsize * sizeof(unsigned char));
    unsigned char* buff = (unsigned char*) malloc(recordsize * sizeof(unsigned char));
    int minimum_position;

    for(int i = 0; i < records - 1; i++)
    {
        minimum_position = i;
        lseek(sorted_file, i * recordsize, SEEK_SET);

        if(read(sorted_file, smallest_record, recordsize) != recordsize)
        {
            perror("Error with reading from file!");
            return 1;
        }

        for(int j = i + 1; j < records; j++)
        {
            if(read(sorted_file, buff, recordsize) != recordsize)
            {
                perror("Error with reading from file!");
                return 1;
            }

            if(smallest_record[SORT_COMPARED_INDEX] > buff[SORT_COMPARED_INDEX])
            {
                unsigned char* tmp = smallest_record;
                smallest_record = buff;
                buff = tmp;
                minimum_position = j;
            }
        }

        if(minimum_position != i)
        {
            lseek(sorted_file, i * recordsize, SEEK_SET);
            read(sorted_file, buff, recordsize); // write first record to buffer before swapping

            lseek(sorted_file, i * recordsize, SEEK_SET); // we need to return after reading
            if(write(sorted_file, smallest_record, recordsize) != recordsize)
            {
                perror("Error with writing in file!");
                return 1;
            }

            lseek(sorted_file, minimum_position * recordsize, SEEK_SET); // jump to position where i-th record must be moved
            if(write(sorted_file, buff, recordsize) != recordsize)
            {
                perror("Error with writing in file!");
                return 1;
            }
        }
    }

    free(smallest_record);
    free(buff);
    close(sorted_file);
    return 0;
}

int sort_file_lib(char* filename, int records, int recordsize)
{
    FILE* sorted_file = fopen(filename, "r+b");
    if(sorted_file == NULL)
    {
        perror("Error with opening file!");
        errno = 0;
        return 1;
    }

    unsigned char* smallest_record = (unsigned char*) malloc(recordsize * sizeof(char));
    unsigned char* buff = (unsigned char*) malloc(recordsize * sizeof(char));
    int minimum_position;

    for(int i = 0; i < records - 1; i++)
    {
        minimum_position = i;
        fseek(sorted_file, i * recordsize, 0); // 0 - offset counted from beginning of file

        if(fread(smallest_record, sizeof(char), recordsize, sorted_file) != recordsize)
        {
            perror("Error with reading from file!");
            return 1;
        }

        for(int j = i + 1; j < records; j++)
        {
            if(fread(buff, sizeof(char), recordsize, sorted_file) != recordsize)
            {
                perror("Error with reading from file!");
                return 1;
            }

            if(smallest_record[SORT_COMPARED_INDEX] > buff[SORT_COMPARED_INDEX])
            {
                unsigned char* tmp = smallest_record;
                smallest_record = buff;
                buff = tmp;
                minimum_position = j;
            }
        }

        if(minimum_position != i)
        {
            fseek(sorted_file, i * recordsize, 0);
            fread(buff, sizeof(char), recordsize, sorted_file);

            fseek(sorted_file, i * recordsize, 0);
            if(fwrite(smallest_record, sizeof(char), recordsize, sorted_file) != recordsize)
            {
                perror("Error with writing in file!");
                return 1;
            }

            fseek(sorted_file, minimum_position * recordsize, 0);
            if(fwrite(buff, sizeof(char), recordsize, sorted_file) != recordsize)
            {
                perror("Error with writing in file!");
                return 1;
            }
        }
    }

    free(smallest_record);
    free(buff);
    fclose(sorted_file);
    return 0;
}

int copy_file_sys(char* source_name, char* destination_name, int records, int recordsize)
{
    int source_file = open(source_name, O_RDONLY);
    if(source_file == -1)
    {
        perror("Error with opening source file!");
        errno = 0;
        close(source_file);
        return 1;
    }

    int destination_file = open(destination_name, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if(errno == EEXIST)
    {
        perror("Destination file already exists!");
        errno = 0;
        close(destination_file);
        return 1;
    }
    if(destination_file == -1)
    {
        perror("Error with opening destination file!");
        errno = 0;
        close(destination_file);
        return 1;
    }

    char* buff = (char*) malloc(recordsize * sizeof(char));

    for(int i = 0; i < records; i++)
    {
        if(read(source_file, buff, recordsize) != recordsize)
        {
            perror("Error with reading from source!");
            return 1;
        }

        if(write(destination_file, buff, recordsize) != recordsize)
        {
            perror("Error with writing to destination file!");
            return 1;
        }
    }

    close(source_file);
    close(destination_file);
    free(buff);

    return 0;
}

int copy_file_lib(char* source_name, char* destination_name, int records, int recordsize)
{
    FILE* source_file = fopen(source_name, "rb");
    FILE* destination_file = fopen(destination_name, "wb");

    if(source_file == NULL)
    {
        perror("Error with opening source file!");
        errno = 0;
        return 1;
    }

    if(destination_file == NULL)
    {
        perror("Error with opening destination file!");
        errno = 0;
        return 1;
    }

    char* buff = (char*) malloc(recordsize * sizeof(char));

    for(int i = 0; i < records; i++)
    {
        if(fread(buff, sizeof(char), recordsize, source_file) != recordsize)
        {
            perror("Error with reading from source!");
            return 1;
        }

        if(fwrite(buff, sizeof(char), recordsize, destination_file) != recordsize)
        {
            perror("Error with writing to destination file!");
            return 1;
        }
    }

    fclose(source_file);
    fclose(destination_file);
    free(buff);

    return 0;
}

int main(int argc, char* argv[])
{
    if(argc < ARGC_FOR_GENERATE || argc > ARGC_FOR_COPY)
    {
        errno = EINVAL;
        perror("Incorrect number of arguments!\n");
        return 1;
    }

    enum function* f = malloc(sizeof(enum function));
    enum mode* m = malloc(sizeof(enum mode));

    if(get_function_and_mode(argc, argv, f, m) != 0)
        return 1;

    clock_t start_point = 0;
    struct tms* tms_start_point = (struct tms*) malloc(sizeof(struct tms));
    clock_t end_point = 0;
    struct tms* tms_end_point = (struct tms*) malloc(sizeof(struct tms));

    switch(*f)
    {
        case generate:
        {
            char* filename = NULL;
            int records = 0;
            int recordsize = 0;

            read_generate_parameters(argv, &filename, &records, &recordsize);
            if(check_generate_sort_parameters(filename, records, recordsize) != 0)
                return 1;

            printf("\nGenerating:\n");
            printf("\tfile name: %s\n\tnumber of records: %d\n\tsize of single record: %d\n", filename, records, recordsize);
            generate_record_file(filename, records, recordsize);

            break;
        }
        case sort:
        {
            char* filename = NULL;
            int records = 0;
            int recordsize = 0;

            read_sort_parameters(argv, &filename, &records, &recordsize);
            if(check_generate_sort_parameters(filename, records, recordsize) != 0)
                return 1;

            printf("\nSorting:\n");
            printf("\tfile name: %s\n", filename);
            printf("\tnumber of records: %d\n", records);
            printf("\tsize of single record: %d\n", recordsize);

            switch(*m)
            {
                case sys:
                    printf("\tmode: sys\n");

                    start_point = times(tms_start_point);
                    if(sort_file_sys(filename, records, recordsize) != 0)
                        return 1;
                    end_point = times(tms_end_point);
                    print_times(start_point, tms_start_point, end_point, tms_end_point);

                    break;
                case lib:
                    printf("\tmode: lib\n");

                    start_point = times(tms_start_point);
                    if(sort_file_lib(filename, records, recordsize) != 0)
                        return 1;
                    end_point = times(tms_end_point);
                    print_times(start_point, tms_start_point, end_point, tms_end_point);

                    break;
                default:
                    perror("Error with getting mode!");
                    return 1;
            }
            break;
        }
        case copy:
        {
            char* source_file = NULL;
            char* destination_file = NULL;
            int records = 0;
            int recordsize = 0;

            read_copy_parameters(argv, &source_file, &destination_file, &records, &recordsize);
            if(check_copy_parameters(source_file, destination_file, records, recordsize) != 0)
                return 1;

            printf("\nCopying:\n");
            printf("\tfrom file: %s\n", source_file);
            printf("\tto file: %s\n", destination_file);
            printf("\tnumber of records: %d\n", records);
            printf("\tsize of single record: %d\n", recordsize);

            switch(*m)
            {
                case sys:
                    printf("\tmode: sys\n");

                    start_point = times(tms_start_point);
                    if(copy_file_sys(source_file, destination_file, records, recordsize) != 0)
                        return 1;
                    end_point = times(tms_end_point);
                    print_times(start_point, tms_start_point, end_point, tms_end_point);

                    break;
                case lib:
                    printf("\tmode: lib\n");

                    start_point = times(tms_start_point);
                    if(copy_file_lib(source_file, destination_file, records, recordsize) != 0)
                        return 1;
                    end_point = times(tms_end_point);
                    print_times(start_point, tms_start_point, end_point, tms_end_point);

                    break;
                default:
                    perror("Error with getting mode!");
                    return 1;
            }
            break;
        }
        default:
        {
            perror("Error with getting function!");
            return 1;
        }
    }

    return 0;
}