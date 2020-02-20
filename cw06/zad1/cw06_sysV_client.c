#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>

#include "cw06_sysV_chat.h"

#define COMMAND_LENGTH 10
#define ECHO_LENGTH 5
#define FRIENDS_LENGTH 8
#define ADD_LENGTH 4
#define DEL_LENGTH 4
#define _2ALL_LENGTH 5
#define _2FRIENDS_LENGTH 9
#define _2ONE_LENGTH 5
#define DELIMITER " "

int clientqid = -1;
int serverqid = -1;
int clientid = -1;
int is_working = 1;

void handle_commands(char* full_command); // declaration of signature to avoid warnings

void remove_client_queue(void)
{
    if(msgctl(clientqid, IPC_RMID, NULL) != 0)
    {
        perror("Error with deleting client queue!");
    }
}

int create_client_queue(key_t* client_queue_key, key_t* server_queue_key)
{
    char* env_val = getenv(ENV_VAR_FOR_FTOK);
    if(env_val == NULL)
    {
        errno = ENODATA;
        perror("$HOME variable does not exist!");
        return -1;
    }

    *client_queue_key = ftok(env_val, getpid());
    *server_queue_key = ftok(env_val, FTOK_LETTER);
    if(*client_queue_key == -1 || *server_queue_key == -1)
    {
        perror("Error with generating client/server queue key!");
        return -1;
    }

    clientqid = msgget(*client_queue_key, IPC_CREAT | IPC_EXCL | MSGGET_PERMISSIONS);
    if(clientqid == -1)
    {
        perror("Error with creating client queue!");
        return -1;
    }

    serverqid = msgget(*server_queue_key, 0);
    if(serverqid == -1)
    {
        perror("Error with opening server queue!");
        return -1;
    }

    return 0;
}

int init_client(key_t client_queue_key)
{
    struct message msg;
    msg.type = INIT;
    sprintf(msg.msg, "%d", client_queue_key);
    msg.client_id = getpid();

    if(msgsnd(serverqid, &msg, MSG_SIZE, IPC_NOWAIT) != 0)
    {
        perror("Error with sending INIT message!");
        return -1;
    }

    if(msgrcv(clientqid, &msg, MSG_SIZE, INIT, 0) == -1)
    {
        perror("Error with receiving message from server!");
        return -1;
    }

    clientid = atoi(msg.msg);
    return 0;
}

int send_message_to_server(enum message_type msg_type, char* msg_text)
{
    struct message msg;
    msg.client_id = clientid;
    msg.type = msg_type;
    strcpy(msg.msg, msg_text);

    if(msgsnd(serverqid, &msg, MSG_SIZE, IPC_NOWAIT) != 0)
    {
        perror("Error with sending message to server!");
        return -1;
    }

    return 0;
}

char* determine_args_number(char* full_command, int desired_minimum) // returns NULL if command contains only one part without arguments
{
    // array for copy before doing strtok
    char* full_command_copy = malloc((strlen(full_command) + 1) * sizeof(char));
    if(full_command_copy == NULL)
    {
        perror("Error with memory allocation for copy of command in friends!");
        return NULL;
    }
    strcpy(full_command_copy, full_command);

    char* tmp = strtok(full_command_copy, DELIMITER);
    for(int i = 1; i < desired_minimum; i++)
    {
        tmp = strtok(NULL, DELIMITER);
    }

    return tmp;
}

void handle_echo(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL)
    {
        perror("Wrong command!");
        return;
    }

    if(send_message_to_server(ECHO, full_command + ECHO_LENGTH) != 0)
        return;
    
    struct message received_msg;
    if(msgrcv(clientqid, &received_msg, MSG_SIZE, ECHO, 0) == -1)
    {
        perror("Error with receiving echo message from server!");
        return;
    }

    printf("%s", received_msg.msg);
}

void handle_list(void)
{
    if(send_message_to_server(LIST, "") != 0)
        return;
    
    struct message received_msg;
    if(msgrcv(clientqid, &received_msg, MSG_SIZE, LIST, 0) == -1)
    {
        perror("Error with receiving list message from server!");
        return;
    }

    printf("Active clients:\n%s", received_msg.msg);
}

void handle_friends(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL) // only FRIENDS without further arguments
    {
        send_message_to_server(FRIENDS, BLANK_FRIENDS_STR);
    }
    else // FRIENDS with friends list
    {
        send_message_to_server(FRIENDS, full_command + FRIENDS_LENGTH);
    }
}

void handle_add(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL) // no new friends IDs given
    {
        errno = ENODATA;
        perror("No client IDs for ADD given!");
        return;
    }
    else // ADD with new friends list
    {
        send_message_to_server(ADD, full_command + ADD_LENGTH);
    }
}

void handle_del(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL) // no friends to delete given
    {
        errno = ENODATA;
        perror("No client IDs for DEL given!");
        return;
    }
    else
    {
        send_message_to_server(DEL, full_command + DEL_LENGTH);
    }
}

void handle_2all(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL) // no message given
    {
        errno = ENODATA;
        perror("No message for 2ALL given!");
        return;
    }
    else
    {
        send_message_to_server(_2ALL, full_command + _2ALL_LENGTH);
    }
    
}

void handle_2friends(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL) // no message given
    {
        errno = ENODATA;
        perror("No message for 2FRIENDS given!");
        return;
    }
    else
    {
        send_message_to_server(_2FRIENDS, full_command + _2FRIENDS_LENGTH);
    }
}

void handle_2one(char* full_command)
{
    if(determine_args_number(full_command, 3) == NULL) // no message given
    {
        errno = ENODATA;
        perror("No message for 2ONE given!");
        return;
    }
    else
    {
        send_message_to_server(_2ONE, full_command + _2ONE_LENGTH);
    }
}

void handle_stop(int signum)
{
    send_message_to_server(STOP, "");
    exit(EXIT_SUCCESS);
}

void handle_read(char* full_command)
{
    if(determine_args_number(full_command, 2) == NULL)
    {
        errno = ENODATA;
        perror("No filename for READ given!");
        return;
    }

    char* file_name = strtok(full_command, DELIMITER);
    file_name = strtok(NULL, DELIMITER);
    for(int i = 0; i < strlen(file_name); i++)
    {
        if(file_name[i] == '\n')
            file_name[i] = '\0';
    }

    FILE* input_file = fopen(file_name, "r");
    if(input_file == NULL)
    {
        perror("Error with opening input file!");
        return;
    }

    char* current_command = malloc(MAX_MESSAGE_LENGTH * sizeof(char));
    while(fgets(current_command, MAX_MESSAGE_LENGTH, input_file) != NULL)
    {
        handle_commands(current_command);
    }
    free(current_command);
    fclose(input_file);
}

void handle_commands(char* full_command)
{
    char command_type[COMMAND_LENGTH];

    if(sscanf(full_command, "%s", command_type) == EOF)
    {
        errno = ENODATA;
        perror("Wrong command!");
    }

    if(strcmp(command_type, "ECHO") == 0)
    {
        handle_echo(full_command);
    }
    else if(strcmp(command_type, "LIST") == 0)
    {
        handle_list();
    }
    else if(strcmp(command_type, "FRIENDS") == 0)
    {
        handle_friends(full_command);
    }
    else if(strcmp(command_type, "ADD") == 0)
    {
        handle_add(full_command);
    }
    else if(strcmp(command_type, "DEL") == 0)
    {
        handle_del(full_command);
    }
    else if(strcmp(command_type, "2ALL") == 0)
    {
        handle_2all(full_command);
    }
    else if(strcmp(command_type, "2FRIENDS") == 0)
    {
        handle_2friends(full_command);
    }
    else if(strcmp(command_type, "2ONE") == 0)
    {
        handle_2one(full_command);
    }
    else if(strcmp(command_type, "STOP") == 0)
    {
        handle_stop(0); // 0 just to have int, handle_stop is a function which also takes the SIGINT signal, so it has to take int as an argument
    }
    else if(strcmp(command_type, "READ") == 0)
    {
        handle_read(full_command);
    }
    else
    {
        errno = ENODATA;
        perror("Unrecognized command!");
    }    
}

void take_sigusr1(int signum)
{
    struct msqid_ds queue_info;
    if(msgctl(clientqid, IPC_STAT, &queue_info) != 0)
    {
        perror("Error with obtaining client queue information!");
    }

    unsigned short message_number = queue_info.msg_qnum;

    for(int i = 0; i < message_number; i++)
    {
        struct message received_msg;

        if(msgrcv(clientqid, &received_msg, MSG_SIZE, -TYPES_NUMBER, 0) == -1)
        {
            perror("Error with receiving message signalized with SIGUSR1!");
            return;
        }

        if(received_msg.type == _2ALL)
        {
            printf("[PUBLIC] %s", received_msg.msg);
        }
        else if(received_msg.type == _2FRIENDS)
        {
            printf("[FRIEND LIST] %s", received_msg.msg);
        }
        else if(received_msg.type == _2ONE)
        {
            printf("[PRIVATE] %s", received_msg.msg);
        }
        else if(received_msg.type == STOP)
        {
            handle_stop(0); // 0 as it was described above
        }
        else
        {
            errno = ENODATA;
            perror("Unrecognized message type!");
        }
    }
}

int main(int argc, char* argv[])
{
    if(atexit(remove_client_queue) != 0)
    {
        perror("Error with setting atexit function for client!");
        return 1;
    }

    struct sigaction client_sigint_sigaction;
    client_sigint_sigaction.sa_handler = handle_stop;
    sigemptyset(&client_sigint_sigaction.sa_mask);
    client_sigint_sigaction.sa_flags = 0;
    if(sigaction(SIGINT, &client_sigint_sigaction, NULL) != 0)
    {
        perror("Error with setting function to handle SIGINT!");
        return 1;
    }

    struct sigaction client_sigusr1_sigaction;
    client_sigusr1_sigaction.sa_handler = take_sigusr1;
    sigemptyset(&client_sigusr1_sigaction.sa_mask);
    client_sigusr1_sigaction.sa_flags = 0;
    if(sigaction(SIGUSR1, &client_sigusr1_sigaction, NULL) != 0)
    {
        perror("Error with setting function to handle SIGUSR1!");
        return 1;
    }

    key_t client_queue_key, server_queue_key;

    if(create_client_queue(&client_queue_key, &server_queue_key) != 0)
        return 1;

    if(init_client(client_queue_key) != 0)
        return 1;
    
    char command_buffer[MAX_MESSAGE_LENGTH];
    FILE* input = fdopen(STDIN_FILENO, "r");
    if(input == NULL)
    {
        perror("Error with opening standard input!");
        return 1;
    }

    while(is_working == 1)
    {
        while(fgets(command_buffer, MAX_CLIENTS_NUMBER * sizeof(char), input) == NULL); // to avoid having NULL commands after printf
        handle_commands(command_buffer);
    }
    fclose(input);

    return 0;
}