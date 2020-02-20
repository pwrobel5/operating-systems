#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ftw.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MINIMAL_NUMBER_OF_ARGUMENTS 3
#define DIRECTORY_PATH_POSITION 1
#define MODE_POSITION 2

#define LS_LENGTH 6
#define LS_COMMAND "ls -l "

enum mode {nftw_func, other_funcs};

void dir_functions_search(char* directory_path)
{
    if(directory_path == NULL)
    {
        printf("Directory path NULL!\n");
        return;
    }
        
    DIR* directory = opendir(directory_path);
    if(directory == NULL)
    {
        printf("Error with opening directory: %s\n", directory_path);
        return;
    }    

    pid_t child_pid = fork();
    if(child_pid == 0)
    {
        printf("Directory path: %s\n", directory_path);
        printf("PID: %d\n", (int) getpid());

        execlp("ls", "ls", "-l", directory_path, NULL);    
    }
    else
    {
        wait(NULL);
    }    

    struct dirent *next_file = readdir(directory);

    while(next_file != NULL)
    {
        int new_path_size = strlen(directory_path) + strlen(next_file->d_name) + 2; // 1 for null byte, 1 for '/' sign
        char* new_path = (char*) malloc(new_path_size * sizeof(char));

        strcpy(new_path, directory_path);
        strcat(new_path, "/");
        strcat(new_path, next_file->d_name);

        struct stat* current_file_stat = (struct stat*) malloc(sizeof(struct stat));
        lstat(new_path, current_file_stat);
        
        if(strcmp(next_file->d_name, "..") != 0 && strcmp(next_file->d_name, ".") != 0 && S_ISDIR(current_file_stat->st_mode))
        {
            dir_functions_search(new_path);
        }

        free(new_path);
        free(current_file_stat);

        next_file = readdir(directory);
    }

    closedir(directory);    
}

int nftw_util(const char* fpath, const struct stat* st, int tflag, struct FTW* ftwbuf)
{
    if(tflag != FTW_D)
        return 0;
    
    pid_t child_pid = fork();
    if(child_pid == 0){
        printf("Directory path: %s\n", fpath);
        printf("PID: %d\n", (int) getpid());

        execlp("ls", "ls", "-l", fpath, NULL);
    }  
    else
    {
        wait(NULL);
    }     

    return 0;
}

int parse_input_data(int argc, char* argv[], char** directory_path, enum mode* m)
{
    if(argc != MINIMAL_NUMBER_OF_ARGUMENTS)
    {
        printf("Wrong arguments number!\n");
        return 1;
    }

    *directory_path = argv[DIRECTORY_PATH_POSITION];
    
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
    enum mode *m = (enum mode *) malloc(sizeof(enum mode));

    if(parse_input_data(argc, argv, &directory_path, m) != 0)
        return 1;
    
    
    switch (*m) {
        case other_funcs: {
            dir_functions_search(directory_path);
            break;
        }
        case nftw_func: {
            int fd_limit = 5;
            int flags = FTW_PHYS;

            nftw(directory_path, nftw_util, fd_limit, flags);
            break;
        }
        default:
            printf("Error with mode!\n");
            return 1;
    }

    return 0;
}