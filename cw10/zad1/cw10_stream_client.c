#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cw10_stream.h"

#define UNIX_NUMBER_OF_ARGUMENTS 4
#define NET_NUMBER_OF_ARGUMENTS 5

#define CLIENT_NAME_POSITION 1
#define MODE_POSITION 2
#define UNIX_SOCKET_PATH_POSITION 3
#define NET_IPV4_POSITION 3
#define NET_PORT_POSITION 4

#define UINX_MODE_TXT "unix"
#define NET_MODE_TXT "net"

#define DOWN_PORT_LIMIT 1024
#define UP_PORT_LIMIT 65535

char* client_name = NULL;
enum Connection_type mode;
char* unix_path = NULL;
struct in_addr net_server_ip;
uint16_t net_server_port = -1;
int server_socket_fd = -1;
int client_working = 1;

void to_lower_case(char* input_text)
{
    int i = 0;
    while(input_text[i] != '\0')
    {
        input_text[i] = tolower(input_text[i]);
        i++;
    }
}

void clean(void)
{
    client_working = 0;

    if(shutdown(server_socket_fd, SHUT_RDWR) != 0)
        raise_error("Error with shutting down connection with socket");
    
    if(close(server_socket_fd) != 0)
        raise_error("Error with closing connection with socket");

    if(client_name != NULL) free(client_name);
    if(unix_path != NULL) free(unix_path);
}

void parse_initial_arguments(int argc, char* argv[])
{
    if(argc < UNIX_NUMBER_OF_ARGUMENTS)
    {
        errno = EINVAL;
        raise_error("Too few arguments given");
    }

    client_name = malloc((strlen(argv[CLIENT_NAME_POSITION]) + 1) * sizeof(char));
    if(client_name == NULL)
        raise_error("Error with memory allocation for client name");
    strcpy(client_name, argv[CLIENT_NAME_POSITION]);

    to_lower_case(argv[MODE_POSITION]);
    if(strcmp(argv[MODE_POSITION], UINX_MODE_TXT) == 0 && argc == UNIX_NUMBER_OF_ARGUMENTS)
    {
        mode = UNIX;
        
        unix_path = malloc((strlen(argv[UNIX_SOCKET_PATH_POSITION]) + 1) * sizeof(char));
        if(unix_path == NULL)
            raise_error("Error with memory allocation for UNIX path");
        strcpy(unix_path, argv[UNIX_SOCKET_PATH_POSITION]);
    }
    else if(strcmp(argv[MODE_POSITION], NET_MODE_TXT) == 0 && argc == NET_NUMBER_OF_ARGUMENTS)
    {
        mode = NET;

        if(inet_aton(argv[NET_IPV4_POSITION], &net_server_ip) == 0)
        {
            errno = EINVAL;
            raise_error("Wrong IP given");
        }
        
        net_server_port = (uint16_t) atoi(argv[NET_PORT_POSITION]);
        if(net_server_port < DOWN_PORT_LIMIT || net_server_port > UP_PORT_LIMIT)
        {
            errno = EINVAL;
            raise_error("Wrong port number given");
        }
    }
    else
    {
        errno = EINVAL;
        raise_error("Wrong arguments given");
    }    
}

void handle_sigint(int signum)
{
    printf("\nSIGINT received, terminating client...\n");
    send_message(server_socket_fd, DELETE_CLIENT, (uint16_t) (strlen(client_name) + 1), client_name);
    exit(EXIT_SUCCESS);
}

void set_exit_handlers(void)
{
    if(atexit(clean) != 0)
        raise_error("Error with setting atexit function");
    
    struct sigaction client_sigact;
    client_sigact.sa_handler = handle_sigint;
    sigemptyset(&client_sigact.sa_mask);
    client_sigact.sa_flags = 0;
    if(sigaction(SIGINT, &client_sigact, NULL) != 0)
        raise_error("Error with setting SIGINT handler");
}

void initialize_sockets(void)
{
    if(mode == UNIX)
    {
        server_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(server_socket_fd == -1)
            raise_error("Error with creating UNIX socket");

        struct sockaddr_un unix_socket_addr;
        unix_socket_addr.sun_family = AF_UNIX;
        strcpy(unix_socket_addr.sun_path, unix_path);

        if(connect(server_socket_fd, (struct sockaddr*) &unix_socket_addr, sizeof(unix_socket_addr)) != 0)
            raise_error("Error with setting up connection with UNIX socket");
    }
    else if(mode == NET)
    {
        server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(server_socket_fd == -1)
            raise_error("Error with creating net socket");
        
        struct sockaddr_in net_socket_addr;
        net_socket_addr.sin_family = AF_INET;
        net_socket_addr.sin_addr = net_server_ip;
        net_socket_addr.sin_port = htons(net_server_port);

        if(connect(server_socket_fd, (struct sockaddr*) &net_socket_addr, sizeof(net_socket_addr)) != 0)
            raise_error("Error with setting up connection with net socket");
    }
}

void connect_to_server(void)
{
    send_message(server_socket_fd, NEW_CLIENT, (uint16_t) (strlen(client_name) + 1), client_name);

    uint8_t msg_type = -1;
    uint16_t msg_size = -1;
    char msg_text;

    if(read(server_socket_fd, &msg_type, TYPE_LENGTH) != TYPE_LENGTH)
        raise_error("Error with reading server response type");
    
    if(read(server_socket_fd, &msg_size, SIZE_LENGTH) != SIZE_LENGTH)
        raise_error("Error with reading server response size");

    if(read(server_socket_fd, &msg_text, msg_size) != msg_size)
        raise_error("Error with reading server response text");

    switch(msg_type)
    {
        case CLIENT_OVERFLOW:
            errno = EACCES;
            raise_error("Too much clients connected");
            break;
        
        case WRONG_NAME:
            errno = EINVAL;
            raise_error("This client name is already occupied");
            break;

        case ADDED_CLIENT:
            printf("Successfully connected\n");
            break;

        default:
            raise_error("Unrecognized message type");
            break;
        }
}

void complete_task(struct Task task)
{
    char cmd_buffer[MAX_COMMAND_SIZE];
    char res_buffer[MAX_FILE_SIZE];
    char whole_buffer[MAX_COMMAND_SIZE];
    
    for(int i = 0; i < MAX_COMMAND_SIZE; i++)
    {
        cmd_buffer[i] = '\0';
        whole_buffer[i] = '\0';
    }
    for(int i = 0; i < MAX_FILE_SIZE; i++)
    {
        res_buffer[i] = '\0';
    }

    sprintf(cmd_buffer, "cat %s | awk '{for(x=1;$x;++x)print $x}' | sort | uniq -c | sort -g -r", task.file_path);

    FILE* partial_res = popen(cmd_buffer, "r");
    if(partial_res == NULL)
        raise_error("Error with executing first command");
    
    fread(res_buffer, sizeof(char), MAX_FILE_SIZE, partial_res);

    if(pclose(partial_res) == -1)
        raise_error("Error with closing command stream");
    
    sprintf(cmd_buffer, "wc -w %s", task.file_path);
    partial_res = popen(cmd_buffer, "r");
    if(partial_res == NULL)
        raise_error("Error with executing first command");
    
    fread(whole_buffer, sizeof(char), MAX_COMMAND_SIZE, partial_res);

    if(pclose(partial_res) == -1)
        raise_error("Error with closing command stream");
    
    strcat(res_buffer, whole_buffer);

    struct Task_res result;
    result.id = task.id;
    strcpy(result.author_name, client_name);
    strcpy(result.result, res_buffer);

    send_message(server_socket_fd, TASK_RESULT, (uint16_t) sizeof(struct Task_res), &result);
}

void read_messages(void)
{
    uint8_t msg_type = -1;
    uint16_t msg_size = -1;
    char buffer;
    struct Task task;

    while(client_working == 1)
    {
        if(read(server_socket_fd, &msg_type, TYPE_LENGTH) != TYPE_LENGTH)
            raise_error("Error with reading message type");
        
        if(read(server_socket_fd, &msg_size, SIZE_LENGTH) != SIZE_LENGTH)
            raise_error("Error with reading message size");
        
        switch(msg_type)
        {
            case TASK:               
                if(read(server_socket_fd, &task, msg_size) != msg_size)
                    raise_error("Error with reading task details");
                
                complete_task(task);
                break;
            
            case SERVER_DISCONNECT:
                if(read(server_socket_fd, &buffer, msg_size) != msg_size)
                    raise_error("Error with reading server message");
                printf("Server disconnected, terminating client...\n");
                exit(EXIT_SUCCESS);
                break;
            
            case PING_SERVER:
                if(read(server_socket_fd, &buffer, msg_size) != msg_size)
                    raise_error("Error with reading server message");

                send_message(server_socket_fd, PING_CLIENT, (uint16_t) (strlen(client_name) + 1), client_name);
                break;

            default:
                raise_error("Unrecognized message type");
                break;
        }
    }
}

int main(int argc, char* argv[])
{
    set_exit_handlers();
    parse_initial_arguments(argc, argv);
    initialize_sockets();
    connect_to_server();
    read_messages();

    return 0;
}
