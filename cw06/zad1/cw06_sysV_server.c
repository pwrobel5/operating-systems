#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#include "cw06_sysV_chat.h"

#define DATE_FORMAT "[%Y-%m-%d_%H-%M-%S]"
#define NUMBER_LENGTH 4
#define DELIMITER " "

int msgqid = -1;
int is_working = 1;
struct client_data clients[MAX_CLIENTS_NUMBER];

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

int create_server_queue(key_t* server_queue_key)
{
    char* env_var = getenv(ENV_VAR_FOR_FTOK);
    if(env_var == NULL)
    {
        errno = ENODATA;
        perror("$HOME variable does not exist!");
        return -1;
    }

    *server_queue_key = ftok(env_var, FTOK_LETTER);
    if(*server_queue_key == -1)
    {
        perror("Error with generating server queue key!");
        return -1;
    }

    msgqid = msgget(*server_queue_key, IPC_CREAT | IPC_EXCL | MSGGET_PERMISSIONS);
    if(msgqid == -1)
    {
        perror("Error with creating server queue!");
        return -1;
    }

    return 0;
}

void remove_server_queue(void)
{
    if(msgctl(msgqid, IPC_RMID, NULL) != 0)
    {
        perror("Error with deleting server queue!");
    }
}

int send_message(enum message_type msg_type, char* msg_text, int client_id)
{
    struct message msg;
    msg.type = msg_type;
    strcpy(msg.msg, msg_text);
    msg.client_id = -1; // it is not important for server -> client communication

    if(msgsnd(clients[client_id].queue_id, &msg, MSG_SIZE, IPC_NOWAIT) != 0)
    {
        perror("Error with sending message for client!");
        return -1;
    }
    return 0;
}

void open_client_queue(struct message* msg)
{
    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_id == -1)
        {
            clients[i].queue_id = msgget(atol(msg->msg), 0);
            if(clients[i].queue_id == -1)
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

            clients[i].client_pid = msg->client_id;

            char tmp_msg[MAX_MESSAGE_LENGTH];
            sprintf(tmp_msg, "%d", i);
            send_message(INIT, tmp_msg, i);
            return;
        }
    }

    perror("Max number of clients reached!");
}

void handle_echo(struct message* msg)
{
    char response[MAX_MESSAGE_LENGTH];
    time_t curr_time = 0;
    time(&curr_time);
    if(strftime(response, MAX_MESSAGE_LENGTH, DATE_FORMAT, localtime(&curr_time)) == 0)
    { 
        perror("Error with formating date to string!");
        return;
    }
    strcat(response, ": ");
    strcat(response, msg->msg);

    send_message(ECHO, response, msg->client_id);
}

void handle_list(struct message* msg)
{
    char response[MAX_MESSAGE_LENGTH];
    char tmp[NUMBER_LENGTH];
    strcpy(response, "");

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_id != -1)
        {
            strcat(response, "\t");
            sprintf(tmp, "%d", i);
            strcat(response, tmp);
            strcat(response, "\n");
        }
    }

    send_message(LIST, response, msg->client_id);
}

void handle_add(struct message* msg)
{
    char* tmp_number;
    tmp_number = strtok(msg->msg, DELIMITER);
    int number = -1;

    while(tmp_number != NULL)
    {
        number = atoi(tmp_number);
        add_to_friend_list(msg->client_id, number);
        tmp_number = strtok(NULL, DELIMITER);
    }
}

void handle_friends(struct message* msg)
{
    delete_friends_list(msg->client_id);

    if(strcmp(msg->msg, BLANK_FRIENDS_STR) != 0)
    {
        handle_add(msg);
    }    
}

void handle_del(struct message* msg)
{
    char* tmp_number;
    tmp_number = strtok(msg->msg, DELIMITER);
    int number = -1;

    while(tmp_number != NULL)
    {
        number = atoi(tmp_number);
        delete_from_friend_list(msg->client_id, number);
        tmp_number = strtok(NULL, DELIMITER);
    }
}

char* make_message(struct message* msg)
{
    char* response = malloc(MAX_MESSAGE_LENGTH * sizeof(char));
    if(response == NULL)
    {
        perror("Error with memory allocation for response string!");
        return NULL;
    }

    strcpy(response, "");
    time_t curr_time = 0;
    time(&curr_time);
    if(strftime(response, MAX_MESSAGE_LENGTH, DATE_FORMAT, localtime(&curr_time)) == 0)
    { 
        perror("Error with formating date to string!");
        return NULL;
    }

    strcat(response, " from ");
    char tmp_num[NUMBER_LENGTH];
    sprintf(tmp_num, "%d", msg->client_id);
    strcat(response, tmp_num);
    strcat(response, ": ");
    strcat(response, msg->msg);

    return response;
}

void handle_2all(struct message* msg)
{
    char* response = make_message(msg);
    if(response == NULL)
        return;

    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_id != -1)
        {
            send_message(_2ALL, response, i);
            kill(clients[i].client_pid, SIGUSR1);
        }
    }

    free(response);
}

void handle_2friends(struct message* msg)
{
    char* response = make_message(msg);
    if(response == NULL)
        return;

    struct node* iterator = clients[msg->client_id].friends_list->next;

    while(iterator != NULL)
    {
        if(clients[iterator->friend_id].queue_id != -1)
        {
            send_message(_2FRIENDS, response, iterator->friend_id);
            kill(clients[iterator->friend_id].client_pid, SIGUSR1);
        }
        iterator = iterator->next;
    }

    free(response);
}

void handle_2one(struct message* msg)
{
    char msg_text_copy[MAX_MESSAGE_LENGTH];
    strcpy(msg_text_copy, msg->msg);

    char* buffer = strtok(msg->msg, DELIMITER);
    int addressee_id = atoi(buffer);
    
    int addressee_id_len = 0;
    int tmp = addressee_id;
    while(tmp != 0)
    {
        tmp /= 10;
        addressee_id_len++;
    }

    strcpy(msg->msg, msg_text_copy + addressee_id_len + 1); // + 1 for space

    char* response = make_message(msg);
    if(response == NULL)
        return;
    
    if(clients[addressee_id].queue_id != -1)
    {
        send_message(_2ONE, response, addressee_id);
        kill(clients[addressee_id].client_pid, SIGUSR1);
    }

    free(response);
}

void handle_stop(struct message* msg)
{
    clients[msg->client_id].client_pid = -1;
    clients[msg->client_id].queue_id = -1;
    delete_friends_list(msg->client_id);
    free(clients[msg->client_id].friends_list);
    clients[msg->client_id].friends_list = NULL;
}

void handle_message(struct message* msg)
{
    if(msg == NULL)
    {
        errno = ENODATA;
        perror("Null message received!");
        return;
    }

    switch(msg->type)
    {
        case INIT:
            open_client_queue(msg);
            break;
        case ECHO:
            handle_echo(msg);
            break;
        case LIST:
            handle_list(msg);
            break;
        case FRIENDS:
            handle_friends(msg);
            break;
        case ADD:
            handle_add(msg);
            break;
        case DEL:
            handle_del(msg);
            break;
        case _2ALL:
            handle_2all(msg);
            break;
        case _2FRIENDS:
            handle_2friends(msg);
            break;
        case _2ONE:
            handle_2one(msg);
            break;
        case STOP:
            handle_stop(msg);
            break;
        default:
            break;
    }
}

void take_sigint(int signum)
{
    for(int i = 0; i < MAX_CLIENTS_NUMBER; i++)
    {
        if(clients[i].queue_id != -1)
        {
            send_message(STOP, "", i);
            kill(clients[i].client_pid, SIGUSR1);

            struct message received_message;
            if(msgrcv(msgqid, &received_message, MSG_SIZE, STOP, 0) == -1)
                perror("Error with receiving STOP signal from client!");
            
            handle_stop(&received_message);
        }
    }

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
        clients[i].queue_id = -1;
        clients[i].client_pid = -1;
        clients[i].friends_list = NULL;
    }

    key_t server_queue_key;
    if(create_server_queue(&server_queue_key) != 0)
        return 1;

    struct message msg_buffer;
    while(is_working == 1)
    {
        if(msgrcv(msgqid, &msg_buffer, MSG_SIZE, -TYPES_NUMBER, 0) == -1) // -TYPES_NUMBER to have appropriate priorities
        {
            perror("Error with receiving message at server side!");
            return 1;
        }
        handle_message(&msg_buffer);
    }
    
    return 0;
}