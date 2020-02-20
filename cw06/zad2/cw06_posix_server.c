#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include "cw06_posix_chat.h"

#define DATE_FORMAT "[%Y-%m-%d_%H-%M-%S]"
#define NUMBER_LENGTH 4
#define DELIMITER " "
#define MSG_SEPARATOR ","

struct client_data {
    mqd_t queue_des;
    pid_t client_pid;
    struct node* friends_list;
};

int serverqdes = -1;
int is_working = 1;
struct client_data clients[MAX_CLIENTS_NUMBER];

void handle_stop(int client_id); // signature placed here to avoid warnings

void delete_friends_list(int client_id)
{
    struct node* head = clients[client_id].friends_list;
    struct node* iterator = head->next;
    head->next = NULL;
    while(iterator != NULL)
    {
        head = iterator;
        iterator = iterator->next;
        free(head);
    }
}

struct node* create_new_node(int friend_id)
{
    struct node* result = malloc(sizeof(struct node));
    if(result == NULL)
    {
        perror("Error with node allocation!");
        return NULL;
    }

    result->friend_id = friend_id;
    result->next = NULL;
    return result;
}

struct node* find_node(int client_id, int friend_id)
{
    struct node* head = clients[client_id].friends_list;
    struct node* iterator = head->next;
    int found = 0;

    while(iterator != NULL && iterator->friend_id <= friend_id && found == 0)
    {
        if(iterator->friend_id == friend_id)
            found = 1;
        else
            iterator = iterator->next;
    }

    return (found == 1) ? iterator : NULL;
}

void add_to_friend_list(int client_id, int friend_id)
{
    if(find_node(client_id, friend_id) != NULL)
        return;
    
    struct node* new_node = create_new_node(friend_id);
    struct node* head = clients[client_id].friends_list;
    struct node* iterator = head->next;

    while(iterator != NULL && iterator->friend_id < friend_id)
    {
        iterator = iterator->next;
        head = head->next;
    }

    new_node->next = head->next;
    head->next = new_node;
}

void delete_from_friend_list(int client_id, int friend_id)
{
    struct node* head = clients[client_id].friends_list;
    struct node* iterator = head->next;
    while(iterator != NULL && iterator->friend_id < friend_id)
    {
        iterator = iterator->next;
        head = head->next;
    }

    if(iterator != NULL && iterator->friend_id == friend_id)
    {
        head->next = iterator->next;
        free(iterator);
    }
}

int send_message(enum message_type msg_type, char* msg, int client_id)
{
    if(mq_send(clients[client_id].queue_des, msg, MAX_MSG_SIZE, priority(msg_type)) != 0)
    {
        perror("Error with sending message to client!");
        return -1;
    }
    return 0;
}

void remove_server_queue(void)
{
    char received_message[MAX_MSG_SIZE];
    unsigned int received_priority;

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_des != -1)
        {
            send_message(STOP, "", i);
            kill(clients[i].client_pid, SIGUSR1);
            handle_stop(i);
            
            if(mq_receive(serverqdes, received_message, MAX_MSG_SIZE, &received_priority) == -1 || received_priority != priority(STOP))
            {
                perror("Error with receiving STOP signal from client!");
            }
        }
    }

    if(mq_close(serverqdes) != 0)
    {
        perror("Error with closing server queue!");
    }

    if(mq_unlink(SERVER_QUEUE_NAME) != 0)
    {
        perror("Error with deleting server queue!");
    }
}

void open_client_queue(pid_t client_pid, char* msg)
{
    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_des == -1)
        {
            clients[i].queue_des = mq_open(msg, O_WRONLY);
            if(clients[i].queue_des == -1)
            {
                perror("Error with opening client queue!");
                return;
            }

            clients[i].friends_list = malloc(sizeof(struct node));
            if(clients[i].friends_list == NULL)
            {
                perror("Error with friends list allocation!");
                return;
            }
            clients[i].friends_list->friend_id = -1;
            clients[i].friends_list->next = NULL;

            clients[i].client_pid = client_pid;

            char tmp_msg[MAX_MSG_SIZE];
            sprintf(tmp_msg, "%d", i);
            send_message(INIT, tmp_msg, i);
            return;
        }
    }

    errno = ENODATA;
    perror("Max number of clients reached!");
}

void handle_echo(int client_id, char* message)
{
    char response[MAX_MSG_SIZE];
    time_t curr_time = 0;
    time(&curr_time);
    if(strftime(response, MAX_MSG_SIZE, DATE_FORMAT, localtime(&curr_time)) == 0)
    { 
        perror("Error with formating date to string!");
        return;
    }
    strcat(response, ": ");
    strcat(response, message);

    send_message(ECHO, response, client_id);
}

void handle_list(int client_id)
{
    char response[MAX_MSG_SIZE];
    char tmp[NUMBER_LENGTH];
    strcpy(response, "");

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_des != -1)
        {
            strcat(response, "\t");
            sprintf(tmp, "%d", i);
            strcat(response, tmp);
            strcat(response, "\n");
        }
    }

    send_message(LIST, response, client_id);
}

void handle_add(int client_id, char* message)
{
    char* tmp_number;
    tmp_number = strtok(message, DELIMITER);
    int number = -1;

    while(tmp_number != NULL)
    {
        number = atoi(tmp_number);
        add_to_friend_list(client_id, number);
        tmp_number = strtok(NULL, DELIMITER);
    }
}

void handle_friends(int client_id, char* message)
{
    delete_friends_list(client_id);

    if(strcmp(message, BLANK_FRIENDS_STR) != 0)
    {
        handle_add(client_id, message);
    }    
}

void handle_del(int client_id, char* message)
{
    char* tmp_number;
    tmp_number = strtok(message, DELIMITER);
    int number = -1;

    while(tmp_number != NULL)
    {
        number = atoi(tmp_number);
        delete_from_friend_list(client_id, number);
        tmp_number = strtok(NULL, DELIMITER);
    }
}

char* make_message(int client_id, char* message)
{
    char* response = malloc(MAX_MSG_SIZE * sizeof(char));
    if(response == NULL)
    {
        perror("Error with memory allocation for response string!");
        return NULL;
    }

    strcpy(response, "");
    time_t curr_time = 0;
    time(&curr_time);
    if(strftime(response, MAX_MSG_SIZE, DATE_FORMAT, localtime(&curr_time)) == 0)
    { 
        perror("Error with formating date to string!");
        return NULL;
    }

    strcat(response, " from ");
    char tmp_num[NUMBER_LENGTH];
    sprintf(tmp_num, "%d", client_id);
    strcat(response, tmp_num);
    strcat(response, ": ");
    strcat(response, message);

    return response;
}

void handle_2all(int client_id, char* message)
{
    char* response = make_message(client_id, message);
    if(response == NULL)
        return;

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_des != -1)
        {
            send_message(_2ALL, response, i);
            kill(clients[i].client_pid, SIGUSR1);
        }
    }

    free(response);
}

void handle_2friends(int client_id, char* message)
{
    char* response = make_message(client_id, message);
    if(response == NULL)
        return;

    struct node* iterator = clients[client_id].friends_list->next;

    while(iterator != NULL)
    {
        if(clients[iterator->friend_id].queue_des != -1)
        {
            send_message(_2FRIENDS, response, iterator->friend_id);
            kill(clients[iterator->friend_id].client_pid, SIGUSR1);
        }
        iterator = iterator->next;
    }

    free(response);
}

void handle_2one(int client_id, char* message)
{
    char msg_text_copy[MAX_MSG_SIZE];
    strcpy(msg_text_copy, message);

    char* buffer = strtok(message, DELIMITER);
    int addressee_id = atoi(buffer);
    
    int addressee_id_len = 0;
    int tmp = addressee_id;
    while(tmp != 0)
    {
        tmp /= 10;
        addressee_id_len++;
    }

    strcpy(message, msg_text_copy + addressee_id_len + 1); // + 1 for space

    char* response = make_message(client_id, message);
    if(response == NULL)
        return;
    
    if(clients[addressee_id].queue_des != -1)
    {
        send_message(_2ONE, response, addressee_id);
        kill(clients[addressee_id].client_pid, SIGUSR1);
    }

    free(response);
}

void handle_stop(int client_id)
{
    if(mq_close(clients[client_id].queue_des) != 0)
    {
        perror("Error with closing client queue!");
    }

    clients[client_id].client_pid = -1;
    clients[client_id].queue_des = -1;
    delete_friends_list(client_id);
    free(clients[client_id].friends_list);
    clients[client_id].friends_list = NULL;
}

void handle_message(char* message)
{
    if(message == NULL)
    {
        errno = ENODATA;
        perror("Null message received!");
        return;
    }

    enum message_type msg_type = atol(strtok(message, MSG_SEPARATOR));
    int client_id = atoi(strtok(NULL, MSG_SEPARATOR));    

    switch(msg_type)
    {
        case INIT:
            open_client_queue(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case ECHO:
            handle_echo(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case LIST:
            handle_list(client_id);
            break;
        case FRIENDS:
            handle_friends(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case ADD:
            handle_add(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case DEL:
            handle_del(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case _2ALL:
            handle_2all(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case _2FRIENDS:
            handle_2friends(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case _2ONE:
            handle_2one(client_id, strtok(NULL, MSG_SEPARATOR));
            break;
        case STOP:
            handle_stop(client_id);
            break;
        default:
            break;
    }
}

void take_sigint(int signum)
{
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    if(atexit(remove_server_queue) != 0)
    {
        perror("Error with setting atexit function for server!");
        return 1;
    }

    struct sigaction server_sigaction;
    server_sigaction.sa_handler = take_sigint;
    sigemptyset(&server_sigaction.sa_mask);
    server_sigaction.sa_flags = 0;
    if(sigaction(SIGINT, &server_sigaction, NULL) != 0)
    {
        perror("Error with setting function to handle SIGINT!");
        return 1;
    }

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        clients[i].queue_des = -1;
        clients[i].client_pid = -1;
        clients[i].friends_list = NULL;
    }

    struct mq_attr server_queue_attr;
    server_queue_attr.mq_maxmsg = MAX_MSG_NUM;
    server_queue_attr.mq_msgsize = MAX_MSG_SIZE;

    serverqdes = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT | O_EXCL, QUEUE_PERMISSIONS, &server_queue_attr);
    if(serverqdes == -1)
    {
        perror("Error with opening server queue!");
        return 1;
    }

    char msg_buffer[MAX_MSG_SIZE];
    while(is_working == 1)
    {
        if(mq_receive(serverqdes, msg_buffer, MAX_MSG_SIZE, NULL) == -2)
        {
            perror("Error with receiving message at server side!");
            return 1;
        }
        handle_message(msg_buffer);
    }

    return 0;
}