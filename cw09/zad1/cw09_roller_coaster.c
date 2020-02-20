#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define CORRECT_ARGUMENTS_NUMBER 5
#define PASSENGER_THREADS_POSITION 1
#define TROLLEY_THREADS_POSITION 2
#define TROLLEY_CAPACITY_POSITION 3
#define NUMBER_OF_RIDES_POSITION 4

#define MAX_MESSAGE_LENGTH 50
#define MAX_IDENTIFIER_LENGTH 15

#define ENTERING_TROLLEY_TIME_SEC 1
#define LEAVING_TROLLEY_TIME_SEC 1
#define MAX_TROLLEY_RIDE_TIME_SEC 10

struct Trolley {
    int current_passengers;
    int* passengers;
};

int passenger_threads = 0;
int trolley_threads = 0;
int trolley_capacity = 0;
int number_of_rides = 0;

struct timeval* start_time = NULL;
int* queue = NULL;
int queue_head = -1;
int queue_tail = 0;
int current_trolley = 0;
int leaving_passenger = -1;
int entering_passenger = -1;
int starting_passenger = -1;
int active_trolleys = 0;
int start_key_pressed = 0;
struct Trolley* trolleys;

char** message_text = NULL;
char** identifier_text = NULL;
int* passenger_indices = NULL;
int* trolley_indices = NULL;
pthread_t* passenger_tids = NULL;
pthread_t* trolley_tids = NULL;

pthread_mutex_t place_at_platform = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t enter_trolley = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t start_key = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t leave_trolley = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t is_this_trolley_turn = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_turn_to_enter_trolley = PTHREAD_COND_INITIALIZER;
pthread_cond_t the_chosen_one_to_start = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_turn_to_leave_trolley = PTHREAD_COND_INITIALIZER;

void raise_error(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void print_message(const char* message, const char* author)
{
    struct timeval* end_time = malloc(sizeof(struct timeval));
    struct timeval* diff_time = malloc(sizeof(struct timeval));
    if(end_time == NULL || diff_time == NULL)
        raise_error("Error with memory allocation for time variables");
    if(gettimeofday(end_time, NULL) != 0)
        raise_error("Error with reading current time");
    
    timersub(end_time, start_time, diff_time);
    printf("[%ld.%06ld][%s] %s\n", diff_time->tv_sec, diff_time->tv_usec, author, message);

    free(end_time);
    free(diff_time);
}

void parse_initial_arguments(int argc, char* argv[])
{
    if(argc != CORRECT_ARGUMENTS_NUMBER)
    {
        errno = EINVAL;
        raise_error("Incorrect number of arguments");
    }
    
    passenger_threads = atoi(argv[PASSENGER_THREADS_POSITION]);
    trolley_threads = atoi(argv[TROLLEY_THREADS_POSITION]);
    trolley_capacity = atoi(argv[TROLLEY_CAPACITY_POSITION]);
    number_of_rides = atoi(argv[NUMBER_OF_RIDES_POSITION]);

    active_trolleys = trolley_threads;

    if(passenger_threads == 0 || trolley_threads == 0 || trolley_capacity == 0 || number_of_rides == 0)
    {
        errno = EINVAL;
        raise_error("Incorrect arguments given");
    }        
    
    if(passenger_threads < trolley_threads * trolley_capacity)
    {
        errno = EINVAL;
        raise_error("Not enough number of passengers");
    }

    queue = malloc(passenger_threads * sizeof(int));
    if(queue == NULL)
        raise_error("Error with memory allocation for passenger queue");

    passenger_indices = malloc(passenger_threads * sizeof(int));
    trolley_indices = malloc(trolley_threads * sizeof(int));
    if(passenger_indices == NULL || trolley_indices == NULL)
        raise_error("Error with memory allocation for passenger/trolley array of indices");
    
    passenger_tids = malloc(passenger_threads * sizeof(pthread_t));
    trolley_tids = malloc(trolley_threads * sizeof(pthread_t));
    if(passenger_tids == NULL || trolley_tids == NULL)
        raise_error("Error with memory allocation for passenger/trolley TIDs");
    
    message_text = malloc((passenger_threads + trolley_threads) * sizeof(char*));
    identifier_text = malloc((passenger_threads + trolley_threads) * sizeof(char*));
    if(message_text == NULL || identifier_text == NULL)
        raise_error("Error with memory allocation for message/identifier texts arrays");

    for(int i = 0; i < passenger_threads; i++)
    {
        passenger_indices[i] = i;
        queue[i] = i;

        message_text[i] = malloc(MAX_MESSAGE_LENGTH * sizeof(char));
        identifier_text[i] = malloc(MAX_IDENTIFIER_LENGTH * sizeof(char));
        if(message_text[i] == NULL || identifier_text[i] == NULL)
            raise_error("Error with memory allocation for message/identifier text string for passenger");
        
        sprintf(identifier_text[i], "PASSENGER %d", i);
    }
    queue_head = 0;
    queue_tail = 0;

    trolleys = malloc(trolley_threads * sizeof(struct Trolley));
    if(trolleys == NULL)
        raise_error("Error with memory allocation for Trolleys array");

    for(int i = 0; i < trolley_threads; i++)
    {
        trolley_indices[i] = i;
        trolleys[i].current_passengers = 0;
        trolleys[i].passengers = malloc(trolley_capacity * sizeof(int));
        if(trolleys[i].passengers == NULL)
            raise_error("Error with memory allocation for passengers of trolley array");
        
        for(int j = 0; j < trolley_capacity; j++)
        {
            trolleys[i].passengers[j] = -1;
        }

        int char_index = i + passenger_threads;
        message_text[char_index] = malloc(MAX_MESSAGE_LENGTH * sizeof(char));
        identifier_text[char_index] = malloc(MAX_IDENTIFIER_LENGTH * sizeof(char));
        if(message_text[char_index] == NULL || identifier_text[char_index] == NULL)
            raise_error("Error with memory allocation for message/identifier text string for trolley");
        
        sprintf(identifier_text[char_index], "TROLLEY %d", i);
    }
}

void put_to_queue(int index)
{
    if(queue_tail == queue_head)
        raise_error("Trying to put to full queue");
    
    if(queue_head == -1)
        queue_head = queue_tail;
    
    queue[queue_tail] = index;
    queue_tail = (queue_tail + 1) % passenger_threads;
}

int pop_from_queue(void)
{
    if(queue_head == -1)
    {   
        errno = EACCES;
        raise_error("Trying to pop from empty queue");
    }

    int result = queue[queue_head];
    queue_head = (queue_head + 1) % passenger_threads;
    if(queue_head == queue_tail) queue_head = -1;

    return result;
}

void press_start_passenger(int index)
{
    pthread_mutex_lock(&start_key);
    while(starting_passenger == -1)
        pthread_cond_wait(&the_chosen_one_to_start, &start_key);
            
    if(starting_passenger == index)
    {
        print_message("Pressing START button", identifier_text[index]);
        start_key_pressed = 1;
        pthread_cond_broadcast(&the_chosen_one_to_start);
    }

    pthread_mutex_unlock(&start_key);
}

void leave_trolley_passenger(int index)
{
    pthread_mutex_lock(&leave_trolley);
    while(leaving_passenger != index)
        pthread_cond_wait(&passenger_turn_to_leave_trolley, &leave_trolley);
            
    sprintf(message_text[index], "Leaving trolley, passengers left inside: %d", trolleys[current_trolley].current_passengers);
    print_message(message_text[index], identifier_text[index]);
    put_to_queue(index);
    sleep(LEAVING_TROLLEY_TIME_SEC);

    leaving_passenger = -1;
    pthread_cond_broadcast(&passenger_turn_to_leave_trolley);
    pthread_mutex_unlock(&leave_trolley);
}

void* passenger(void* passenger_index)
{
    int index = *((int*) passenger_index);
    
    while(active_trolleys > 0)
    {
        pthread_mutex_lock(&enter_trolley);
        while(entering_passenger != index && active_trolleys > 0)
            pthread_cond_wait(&passenger_turn_to_enter_trolley, &enter_trolley);
        
        if(active_trolleys == 0)
        {
            pthread_mutex_unlock(&enter_trolley);
        }
        else
        {            
            sprintf(message_text[index], "Entering trolley, passengers inside: %d", trolleys[current_trolley].current_passengers);
            print_message(message_text[index], identifier_text[index]);
            sleep(ENTERING_TROLLEY_TIME_SEC);

            entering_passenger = -1;
            pthread_cond_broadcast(&passenger_turn_to_enter_trolley);
            pthread_mutex_unlock(&enter_trolley);

            press_start_passenger(index);
            leave_trolley_passenger(index);            
        }
    }

    print_message("No trolley rides available, ending passenger thread work", identifier_text[index]);
    pthread_exit(passenger_index);
}

void move_passengers_out(int index)
{
    int char_index = index + passenger_threads;

    print_message("Opening door", identifier_text[char_index]);

    pthread_mutex_lock(&leave_trolley);
    while(trolleys[index].current_passengers != 0)
    {
        trolleys[index].current_passengers--;
        leaving_passenger = trolleys[index].passengers[trolleys[index].current_passengers];
            
        pthread_cond_broadcast(&passenger_turn_to_leave_trolley);
        while(leaving_passenger != -1)
            pthread_cond_wait(&passenger_turn_to_leave_trolley, &leave_trolley);
    }
    pthread_mutex_unlock(&leave_trolley);
}

void take_passengers_into_trolley(int index)
{
    int char_index = index + passenger_threads;

    pthread_mutex_lock(&enter_trolley);

    while(trolleys[index].current_passengers < trolley_capacity)
    {
        entering_passenger = pop_from_queue();            
        trolleys[index].passengers[trolleys[index].current_passengers] = entering_passenger;
        trolleys[index].current_passengers++;

        pthread_cond_broadcast(&passenger_turn_to_enter_trolley);
        while(entering_passenger != -1)
        {
            pthread_cond_wait(&passenger_turn_to_enter_trolley, &enter_trolley);
        }            
    }        

    pthread_mutex_unlock(&enter_trolley);
    print_message("Closing trolley door", identifier_text[char_index]);
}

void start_trolley_ride(int index, int ride)
{
    int char_index = index + passenger_threads;

    pthread_mutex_lock(&start_key);
    starting_passenger = trolleys[index].passengers[rand() % trolley_capacity];
    start_key_pressed = 0;
    pthread_cond_broadcast(&the_chosen_one_to_start);
    while(start_key_pressed != 1)
    {
        pthread_cond_wait(&the_chosen_one_to_start, &start_key);
    }
    pthread_mutex_unlock(&start_key);

    sprintf(message_text[char_index], "Starting ride number: %d", ride + 1);
    print_message(message_text[char_index], identifier_text[char_index]);
}

void* trolley(void* trolley_index)
{
    int index = *((int*) trolley_index);
    int char_index = index + passenger_threads;
    
    for(int ride = 0; ride < number_of_rides; ride++)
    {
        pthread_mutex_lock(&place_at_platform);
        while(current_trolley != index)
            pthread_cond_wait(&is_this_trolley_turn, &place_at_platform);

        move_passengers_out(index);
        starting_passenger = -1;
        take_passengers_into_trolley(index);       
        start_trolley_ride(index, ride);

        if(ride == number_of_rides - 1)
        {
            active_trolleys--;
            pthread_cond_broadcast(&passenger_turn_to_enter_trolley);
        }

        current_trolley = (current_trolley + 1) % trolley_threads;    
        pthread_cond_broadcast(&is_this_trolley_turn);        
        pthread_mutex_unlock(&place_at_platform);
        
        int ride_time = rand() % MAX_TROLLEY_RIDE_TIME_SEC + 1;
        sleep(ride_time);
        print_message("The ride has ended, waiting for a place at the platform edge", identifier_text[char_index]);        
    }    

    pthread_mutex_lock(&place_at_platform);
    while(current_trolley != index)
        pthread_cond_wait(&is_this_trolley_turn, &place_at_platform);
        
    move_passengers_out(index);

    current_trolley = (current_trolley + 1) % trolley_threads;
    pthread_cond_broadcast(&is_this_trolley_turn);
    pthread_mutex_unlock(&place_at_platform);

    print_message("All rides completed, ending trolley thread work", identifier_text[char_index]);
    pthread_exit(trolley_index);
}

void clean(void)
{
    pthread_mutex_destroy(&place_at_platform);
    pthread_mutex_destroy(&enter_trolley);
    pthread_mutex_destroy(&start_key);
    pthread_mutex_destroy(&leave_trolley);

    pthread_cond_destroy(&is_this_trolley_turn);
    pthread_cond_destroy(&passenger_turn_to_enter_trolley);
    pthread_cond_destroy(&the_chosen_one_to_start);
    pthread_cond_destroy(&passenger_turn_to_leave_trolley);

    free(start_time);

    free(queue);
    free(passenger_tids);
    free(trolley_tids);

    int char_array_length = trolley_threads + passenger_threads;
    for(int i = 0; i < char_array_length; i++)
    {
        free(message_text[i]);
        free(identifier_text[i]);
    }
    free(message_text);
    free(identifier_text);

    for(int i = 0; i < trolley_threads; i++)
    {
        free(trolleys[i].passengers);
    }
    free(trolleys);

    free(passenger_indices);
    free(trolley_indices);
}

int main(int argc, char* argv[])
{
    if(atexit(clean) != 0)
        raise_error("Error with setting atexit function");

    start_time = malloc(sizeof(struct timeval));
    if(start_time == NULL)
        raise_error("Error with memory allocation for start time variable");
    if(gettimeofday(start_time, NULL) != 0)
        raise_error("Error with reading start time");
    
    print_message("Preparing simulation", "MAIN");
    parse_initial_arguments(argc, argv);
    print_message("The simulation begins", "MAIN");

    for(int i = 0; i < passenger_threads; i++)
    {
        if(pthread_create(passenger_tids + i, NULL, &passenger, passenger_indices + i) != 0)
            raise_error("Error with creating passenger thread");
    }

    for(int i = 0; i < trolley_threads; i++)
    {
        if(pthread_create(trolley_tids + i, NULL, &trolley, trolley_indices + i) != 0)
            raise_error("Error with creating trolley thread");
    }

    int* res;
    for(int i = 0; i < passenger_threads; i++)
    {
        if(pthread_join(passenger_tids[i], (void**) &res) != 0)
            raise_error("Error with achieving passenger thread end state");
    }

    for(int i = 0; i < trolley_threads; i++)
    {        
        if(pthread_join(trolley_tids[i], (void**) &res) != 0)
            raise_error("Error with achieving trolley thread end state");
    }

    return 0;  
}