#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include <pthread.h>
#include <sys/time.h>

#define CORRECT_NUMBER_OF_ARGUMENTS 6
#define THREADS_NUM_POSITION 1
#define MODE_POSITION 2
#define IMAGE_FILE_NAME_POSITION 3
#define FILTER_FILE_NAME_POSITION 4
#define OUTPUT_FILE_NAME_POSITION 5

#define BLOCK_MODE_TEXT "block"
#define BLOCK_MODE_NUM 1
#define INTERLEAVED_MODE_TEXT "interleaved"
#define INTERLEAVED_MODE_NUM 2

#define IMAGE_HEADER "P2"
#define MAX_LINE_LENGTH 100
#define DELIMITER " "
#define MAX_IMAGE_COLOURS 255
#define MAX_OUTPUT_LINE_LENGTH 70

enum Mode {
    BLOCK, INTERLEAVED
};

struct thread_arguments {
    int thread_index;
    int** image;
    double** filter;
    int** filtered_image;
    int im_width;
    int im_height;
    int filter_dimension;
};

int** image = NULL;
double** filter = NULL;
int** filtered_image = NULL;

int image_width = 0;
int image_height = 0;
int filter_dimension = 0;
int threads = 0;
int colours = 0;

void raise_error(char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void to_lower_case(char* input_text)
{
    int i = 0;
    while(input_text[i] != '\0')
    {
        input_text[i] = tolower(input_text[i]);
        i++;
    }
}

void parse_initial_arguments(int argc, char* argv[], enum Mode* m, char** image_file_name, char** filter_file_name, char** output_file_name)
{
    if(argc != CORRECT_NUMBER_OF_ARGUMENTS)
    {
        errno = EINVAL;
        raise_error("Incorrect arguments number");
    }

    threads = atoi(argv[THREADS_NUM_POSITION]);
    if(threads <= 0)
    {
        errno = EINVAL;
        raise_error("Incorrect number of threads");
    }

    to_lower_case(argv[MODE_POSITION]);
    if(strcmp(argv[MODE_POSITION], BLOCK_MODE_TEXT) == 0 || atoi(argv[MODE_POSITION]) == BLOCK_MODE_NUM)
        *m = BLOCK;
    else if(strcmp(argv[MODE_POSITION], INTERLEAVED_MODE_TEXT) == 0 || atoi(argv[MODE_POSITION]) == INTERLEAVED_MODE_NUM)
        *m = INTERLEAVED;
    else
    {
        errno = EINVAL;
        raise_error("Wrong mode given");
    }
    
    *image_file_name = malloc((strlen(argv[IMAGE_FILE_NAME_POSITION]) + 1) * sizeof(char));
    if(*image_file_name == NULL)
        raise_error("Error with memory allocation for image file name");
    strcpy(*image_file_name, argv[IMAGE_FILE_NAME_POSITION]);

    *filter_file_name = malloc((strlen(argv[FILTER_FILE_NAME_POSITION]) + 1) * sizeof(char));
    if(*filter_file_name == NULL)
        raise_error("Error with memory allocation for filter file name");
    strcpy(*filter_file_name, argv[FILTER_FILE_NAME_POSITION]);

    *output_file_name = malloc((strlen(argv[OUTPUT_FILE_NAME_POSITION]) + 1) * sizeof(char));
    if(*output_file_name == NULL)
        raise_error("Error with memory allocation for output file name");
    strcpy(*output_file_name, argv[OUTPUT_FILE_NAME_POSITION]);
}

void read_image(char* image_file_name)
{
    char buffer[MAX_LINE_LENGTH];
    FILE* image_file = fopen(image_file_name, "r");
    if(image_file == NULL)
        raise_error("Error with opening image file");

    // read first line of image and check if it's equal to P2
    fscanf(image_file, "%s", buffer);
    if(strcmp(buffer, IMAGE_HEADER) != 0)
    {
        errno = EINVAL;
        raise_error("Incorrect input image file");
    }
    
    // omit comment lines - at first point go to the end of current line, and then omit lines beginning with '#'
    while(getc(image_file) != '\n') asm("nop");
    while(getc(image_file) == '#')
    {
        while(getc(image_file) != '\n')
            asm("nop");
    }
    fseek(image_file, -1, SEEK_CUR); // to place pointer before first line without '#'

    // read width and height of image
    fscanf(image_file, "%d %d %d", &image_width, &image_height, &colours);
    if(image_width == 0 || image_height == 0 || colours == 0)
    {
        errno = EINVAL;
        raise_error("Invalid height, width or max colours value");
    }
    
    image = malloc(image_height * sizeof(int*));
    if(image == NULL)
        raise_error("Error with memory allocation for image input array");
    
    for(int i = 0; i < image_height; i++)
    {
        image[i] = malloc(image_width * sizeof(int));
        if(image[i] == NULL)
            raise_error("Error with memory allocation for image input array (row)");
        
        for(int j = 0; j < image_width; j++)
        {
            fscanf(image_file, "%d", &(image[i][j]));
        }
    }

    if(colours > MAX_IMAGE_COLOURS)
    {
        for(int i = 0; i < image_height; i++)
        {
            for(int j = 0; j < image_width; j++)
            {
                image[i][j] %= MAX_IMAGE_COLOURS;
            }
        }
    }

    fclose(image_file);
}

void read_filter(char* filter_file_name)
{
    FILE* filter_file = fopen(filter_file_name, "r");
    if(filter_file == NULL)
        raise_error("Error with opening filter file");
    
    fscanf(filter_file, "%d", &filter_dimension);

    filter = malloc(filter_dimension * sizeof(double*));
    if(filter == NULL)
        raise_error("Error with memory allocation for filter matrix");
    
    for(int i = 0; i < filter_dimension; i++)
    {
        filter[i] = malloc(filter_dimension * sizeof(double));
        if(filter[i] == NULL)
            raise_error("Error with memory allocation for image filter array");
        for(int j = 0; j < filter_dimension; j++)
        {
            fscanf(filter_file, "%lf", &(filter[i][j]));
        }
    }

    fclose(filter_file);
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

int get_index_for_filter(int a, int b)
{
    return max(0, a - (int) ceil((double) filter_dimension / 2) + b);
}

int apply_filter(int x, int y)
{
    double res = 0.0;
    int first_index = 0;
    int second_index = 0;

    for(int i = 0; i < filter_dimension; i++)
    {
        first_index = get_index_for_filter(x, i);
        if(first_index >= image_height) break;

        for(int j = 0; j < filter_dimension; j++)
        {            
            second_index = get_index_for_filter(y, j);
            if(second_index >= image_width) break;
            res += image[first_index][second_index] * filter[i][j];
        }
    }

    if(res < 0) res = 0.0;
    if(res > colours) res = colours;

    return (int) round(res);
}

void* block_filter(void* thread_index)
{
    struct timeval* start_time = malloc(sizeof(struct timeval));
    struct timeval* end_time = malloc(sizeof(struct timeval));
    struct timeval* diff_time = malloc(sizeof(struct timeval));

    if(start_time == NULL || end_time == NULL || diff_time == NULL) 
        raise_error("Error with timeval structure memory allocation");
    if(gettimeofday(start_time, NULL) != 0)
        raise_error("Error with reading start time");

    int k = *((int*) thread_index) + 1;
    int x_min = (k - 1) * (int) ceil((double) image_width / threads);
    int x_max = k * (int) ceil((double) image_width / threads);
    if(x_max > image_width) x_max = image_width;   // to avoid going out of an array

    for(int i = 0; i < image_height; i++)
    {
        for(int j = x_min; j < x_max; j++)
        {
            filtered_image[i][j] = apply_filter(i, j);
        }
    }

    if(gettimeofday(end_time, NULL) != 0)
        raise_error("Error with reading end time");
    timersub(end_time, start_time, diff_time);

    free(start_time);
    free(end_time);

    pthread_exit(diff_time);
}

void* interleaved_filter(void* thread_index)
{
    struct timeval* start_time = malloc(sizeof(struct timeval));
    struct timeval* end_time = malloc(sizeof(struct timeval));
    struct timeval* diff_time = malloc(sizeof(struct timeval));

    if(start_time == NULL || end_time == NULL || diff_time == NULL) 
        raise_error("Error with timeval structure memory allocation");
    if(gettimeofday(start_time, NULL) != 0)
        raise_error("Error with reading start time");

    int k = *((int*) thread_index) + 1;
    
    for(int i = 0; i < image_height; i++)
    {
        int j = k - 1;
        while(j < image_width)
        {
            filtered_image[i][j] = apply_filter(i, j);
            j += threads;
        }
    }

    if(gettimeofday(end_time, NULL) != 0)
        raise_error("Error with reading end time");
    timersub(end_time, start_time, diff_time);

    free(start_time);
    free(end_time);   

    pthread_exit(diff_time);
}

int number_width(int value)
{
    int result = 0;
    if(value < 0) result++;
    
    do
    {
        result++;
        value /= 10;
    } while (value != 0);    

    return result;
}

void write_output_file(char* output_file_name)
{
    FILE* output_file = fopen(output_file_name, "w");
    if(output_file == NULL)
        raise_error("Error with creating output file");
    
    fprintf(output_file, "%s\n", IMAGE_HEADER);
    fprintf(output_file, "%d %d\n", image_width, image_height);
    fprintf(output_file, "%d\n", (colours > MAX_IMAGE_COLOURS) ? MAX_IMAGE_COLOURS : colours);

    int line_width = 0;

    for(int i = 0; i < image_height; i++)
    {
        for(int j = 0; j < image_width; j++)
        {
            int current_char_width = number_width(filtered_image[i][j]) + 1; // 1 for space
            if(line_width + current_char_width >= MAX_OUTPUT_LINE_LENGTH)
            {
                fprintf(output_file, "\n");
                line_width = 0;
            }
            line_width += current_char_width;
            fprintf(output_file, "%d ", filtered_image[i][j]);
        }
    }

    fclose(output_file);
}

int main(int argc, char* argv[])
{
    enum Mode m;
    char* image_file_name;
    char* filter_file_name;
    char* output_file_name;

    parse_initial_arguments(argc, argv, &m, &image_file_name, &filter_file_name, &output_file_name);
    read_image(image_file_name);
    read_filter(filter_file_name);

    filtered_image = malloc(image_height * sizeof(int*));
    if(filtered_image == NULL)
        raise_error("Error with memory allocation for filtered image");
    for(int i = 0; i < image_height; i++)
    {
        filtered_image[i] = malloc(image_width * sizeof(int));
        if(filtered_image[i] == NULL)
            raise_error("Error with memory allocation for row in filtered image");
    }

    pthread_t* tids = malloc(threads * sizeof(pthread_t));
    if(tids == NULL)
        raise_error("Error with memory allocation for threads' IDs");
    
    int* thread_indices = malloc(threads * sizeof(int));
    if(thread_indices == NULL)
        raise_error("Error with memory allocation for thread indices array");
    for(int i = 0; i < threads; i++) thread_indices[i] = i;

    struct timeval* start_time = malloc(sizeof(struct timeval));
    struct timeval* end_time = malloc(sizeof(struct timeval));
    struct timeval* diff_time = malloc(sizeof(struct timeval));

    if(start_time == NULL || end_time == NULL || diff_time == NULL) 
        raise_error("Error with timeval structure memory allocation");
    if(gettimeofday(start_time, NULL) != 0)
        raise_error("Error with reading start time");

    printf("\tStarting filtration\n");
    printf("\t\tthreads: %d\n\t\tmode: %s\n\t\tfilter dimension: %d\n\n", threads, (m == BLOCK) ? BLOCK_MODE_TEXT : INTERLEAVED_MODE_TEXT, filter_dimension);

    if(m == BLOCK)
    {
        for(int i = 0; i < threads; i++)
        {
            if(pthread_create(tids + i, NULL, &block_filter, thread_indices + i) != 0)
                raise_error("Error with thread creation");
        }
    }
    else
    {
        for(int i = 0; i < threads; i++)
        {
            if(pthread_create(tids + i, NULL, &interleaved_filter, thread_indices + i) != 0)
                raise_error("Error with thread creation");
        }
    }
    
    for(int i = 0; i < threads; i++)
    {
        struct timeval* res;
        if(pthread_join(tids[i], (void**) &res) != 0)
            raise_error("Error with achieving return value from thread");
        printf("\t\tPthread %ld ended, used time: %ld.%06ld s\n", tids[i], res->tv_sec, res->tv_usec);
        free(res);
    }

    if(gettimeofday(end_time, NULL) != 0)
        raise_error("Error with reading end time");
    timersub(end_time, start_time, diff_time);

    printf("\tTotal time: %ld.%06ld s\n", diff_time->tv_sec, diff_time->tv_usec);
    free(start_time);
    free(end_time);
    free(diff_time);

    write_output_file(output_file_name);
    
    for(int i = 0; i < image_height; i++)
    {
        free(image[i]);
        free(filtered_image[i]);
    }
    free(image);
    free(filtered_image);

    for(int i = 0; i < filter_dimension; i++)
        free(filter[i]);
    free(filter);

    free(tids);
    free(thread_indices);

    free(image_file_name);
    free(filter_file_name);
    free(output_file_name);

    return 0;
}