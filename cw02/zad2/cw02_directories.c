#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#define MINIMAL_NUMBER_OF_ARGUMENTS 5
#define DIRECTORY_PATH_POSITION 1
#define OPERATION_POSITION 2
#define DATE_POSITION 3
#define MODE_POSITION 4

#define EQUAL_OPERATION "="
#define SMALLER_OPERATION "<"
#define BIGGER_OPERATION ">"

#define DATE_FORMAT "%d-%m-%Y"
#define DATE_LENGTH 14

enum mode {nftw_func, other_funcs};

time_t date_nftw;
char* operation_nftw;

double time_difference(time_t reference_date, time_t current_date)
{
    return difftime(reference_date, current_date);
}

void print_file_details(const char* file_path, const struct stat* file_stat)
{
    printf("File path: %s\n", file_path);

    printf("File type: ");
    if(S_ISREG(file_stat->st_mode))
        printf("regular file\n");
    else if(S_ISDIR(file_stat->st_mode))
        printf("directory\n");
    else if(S_ISCHR(file_stat->st_mode))
        printf("character device\n");
    else if(S_ISBLK(file_stat->st_mode))
        printf("block device\n");
    else if(S_ISFIFO(file_stat->st_mode))
        printf("FIFO named pipe\n");
    else if(S_ISLNK(file_stat->st_mode))
        printf("symbolic link\n");
    else if(S_ISSOCK(file_stat->st_mode))
        printf("socket\n");

    printf("File size (bytes): %ld\n", file_stat->st_size);

    char* buffer = (char*) malloc(DATE_LENGTH * sizeof(char));
    strftime(buffer, DATE_LENGTH, DATE_FORMAT, localtime(&(file_stat->st_atime)));
    printf("Access date: %s\n", buffer);

    strftime(buffer, DATE_LENGTH, DATE_FORMAT, localtime(&(file_stat->st_mtime)));
    printf("Modification date: %s\n\n", buffer);
}

void dir_functions_search(char* directory_path, char* operation, time_t date)
{
    if(directory_path == NULL)
        return;

    DIR* directory = opendir(directory_path);
    if(directory == NULL)
    {
        printf("Error with opening directory: %s\n", directory_path);
        return;
    }

    struct dirent *next_file = readdir(directory);

    while(next_file != NULL)
    {
        int new_path_size = strlen(directory_path) + strlen(next_file->d_name) + 2; // 2: 1 for '/' sign and 1 for null byte
        char* new_path = (char*) malloc(new_path_size * sizeof(char));

        strcpy(new_path, directory_path);
        strcat(new_path, "/");
        strcat(new_path, next_file->d_name);

        struct stat* current_file_stat = (struct stat*) malloc(sizeof(struct stat));
        lstat(new_path, current_file_stat);

        if(strcmp(next_file->d_name, "..") != 0 && strcmp(next_file->d_name, ".") != 0)
        {
            if(S_ISDIR(current_file_stat->st_mode))
            {
                dir_functions_search(new_path, operation, date);
            }

            if(S_ISREG(current_file_stat->st_mode))
            {
                if(strcmp(BIGGER_OPERATION, operation) == 0 && time_difference(date, current_file_stat->st_mtime) < 0)
                    print_file_details(new_path, current_file_stat);
                else if(strcmp(EQUAL_OPERATION, operation) == 0 && time_difference(date, current_file_stat->st_mtime) == 0)
                    print_file_details(new_path, current_file_stat);
                else if(strcmp(SMALLER_OPERATION, operation) == 0 && time_difference(date, current_file_stat->st_mtime) > 0)
                    print_file_details(new_path, current_file_stat);
            }
        }

        next_file = readdir(directory);
    }

    closedir(directory);
}

int nftw_util(const char* fpath, const struct stat* st, int tflag, struct FTW* ftwbuf)
{
    if(tflag != FTW_D)
        return 0;

    int difference = time_difference(date_nftw, st->st_mtime);

    if((difference < 0 && strcmp(BIGGER_OPERATION, operation_nftw) == 0) || (difference == 0 && strcmp(EQUAL_OPERATION, operation_nftw) == 0) || (difference > 0 && strcmp(SMALLER_OPERATION, operation_nftw) == 0))
    {
        print_file_details(fpath, st);
    }

    return 0;
}

int parse_input_data(int argc, char* argv[], char** directory_path, char** operation, time_t* date, enum mode* m)
{
    if(argc != MINIMAL_NUMBER_OF_ARGUMENTS)
    {
        printf("Wrong arguments number!\n");
        return 1;
    }

    *directory_path = argv[DIRECTORY_PATH_POSITION];
    *operation = argv[OPERATION_POSITION];

    struct tm* tmp_tm = (struct tm*) malloc(sizeof(struct tm));
    strptime(argv[DATE_POSITION], DATE_FORMAT, tmp_tm);
    *date = mktime(tmp_tm);

    if(strcmp(argv[MODE_POSITION], "nftw") == 0)
        *m = nftw_func;
    else if(strcmp(argv[MODE_POSITION], "other") == 0)
        *m = other_funcs;
    else
    {
        printf("Wrong mode!\n");
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    char* directory_path = NULL;
    char* operation = NULL;
    time_t given_date = 0;
    enum mode *m = (enum mode *) malloc(sizeof(enum mode));

    if(parse_input_data(argc, argv, &directory_path, &operation, &given_date, m) != 0)
        return 1;

    switch (*m) {
        case other_funcs: {
            dir_functions_search(realpath(directory_path, NULL), operation, given_date);
            break;
        }
        case nftw_func: {
            date_nftw = given_date;
            operation_nftw = operation;
            int fd_limit = 5;
            int flags = FTW_PHYS;

            nftw(realpath(directory_path, NULL), nftw_util, fd_limit, flags);

            break;
        }
        default:
            printf("Error with mode!\n");
            return 1;
    }

    return 0;
}
