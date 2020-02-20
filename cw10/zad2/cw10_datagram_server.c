#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "cw10_datagram.h"

#define CORRECT_NUMBER_OF_ARGUMENTS 3
#define PORT_NUM_POSITION 1
#define SOCKET_PATH_POSITION 2

#define MAX_CLIENT_CONNECTIONS 32
#define PING_FREQUENCY 10

struct Client_info {
    char* name;
    int is_busy;
    int is_pinged;
    enum Connection_type mode;
    struct sockaddr* client_addr;
    socklen_t socklen;
};

uint16_t port_num = -1;
char* unix_socket_path = NULL;

int unix_socket_fd = -1;
int net_socket_fd = -1;
int epoll_fd = -1;

int server_working = 1;
struct Client_info clients[MAX_CLIENT_CONNECTIONS];
int current_clients = 0;
int id = -1;

pthread_t epoll_thread = -1;
pthread_t terminal_thread = -1;
pthread_t ping_thread = -1;

void parse_initial_arguments(int argc, char* argv[])
{
    if(argc != CORRECT_NUMBER_OF_ARGUMENTS)
    {
        errno = EINVAL;
        raise_error("Invalid number of arguments");
    }

    port_num = (uint16_t) atoi(argv[PORT_NUM_POSITION]);
    
    unix_socket_path = malloc((strlen(argv[SOCKET_PATH_POSITION]) + 1) * sizeof(char));
    if(unix_socket_path == NULL)
        raise_error("Error with memory allocation for UNIX socket path string");
    
    strcpy(unix_socket_path, argv[SOCKET_PATH_POSITION]);

    for(int i = 0; i < MAX_CLIENT_CONNECTIONS; i++)
    {
        clients[i].name = NULL;
        clients[i].is_busy = -1;
        clients[i].is_pinged = -1;
        clients[i].mode = -1;
        clients[i].client_addr = NULL;
        clients[i].socklen = 0;
    }
}

void clean(void)
{
    server_working = 0;   

    if(pthread_cancel(epoll_thread) != 0)
        raise_error("Error with cancelling epoll thread"); 
    
    if(pthread_cancel(terminal_thread) != 0)
        raise_error("Error with cancelling terminal thread");

    if(pthread_cancel(ping_thread) != 0)
        raise_error("Error with cancelling ping thread");    
    
    uint8_t msg_type = SERVER_DISCONNECT;
    for(int i = 0; i < current_clients; i++)
    {
        int client_socket = (clients[i].mode == UNIX) ? unix_socket_fd : net_socket_fd;
        
        if(sendto(client_socket, &msg_type, sizeof(uint8_t), 0, clients[i].client_addr, clients[i].socklen) != sizeof(uint8_t))
            raise_error("Error with sending terminating message to client");

        free(clients[i].name);
        free(clients[i].client_addr);
    }
    
    if(close(unix_socket_fd) != 0)
        raise_error("Error with closing UNIX socket");
    
    if(unlink(unix_socket_path) != 0)
        raise_error("Error with unlinking UNIX socket path");

    if(unix_socket_path != NULL) free(unix_socket_path);
    
    if(close(net_socket_fd) != 0)
        raise_error("Error with closing net socket");
    
    if(close(epoll_fd) != 0)
        raise_error("Error with closing epoll");    
}

void handle_sigint(int signum)
{
    printf("\nSIGINT received, terminating server...\n");
    exit(EXIT_SUCCESS);
}

void set_exit_handlers(void)
{
    if(atexit(clean) != 0)
        raise_error("Erro with setting atexit function");
    
    struct sigaction server_sigact;
    server_sigact.sa_handler = handle_sigint;
    sigemptyset(&server_sigact.sa_mask);
    server_sigact.sa_flags = 0;

    if(sigaction(SIGINT, &server_sigact, NULL) != 0)
        raise_error("Error with setting SIGINT handler");
}

void initialize_sockets(void)
{
    unix_socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(unix_socket_fd == -1)
        raise_error("Error with creating UNIX socket");
    
    struct sockaddr_un unix_socket_addr;
    unix_socket_addr.sun_family = AF_UNIX;
    strcpy(unix_socket_addr.sun_path, unix_socket_path);
    
    if(bind(unix_socket_fd, (struct sockaddr*) &unix_socket_addr, sizeof(unix_socket_addr)) != 0)
        raise_error("Error with binding UNIX socket"); 

    net_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(net_socket_fd == -1)
        raise_error("Error with creating net socket");

    int reuseaddr_val = 1;
    if(setsockopt(net_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_val, sizeof(reuseaddr_val)) != 0)
        raise_error("Error with setting REUSEADDR for net socket");

    struct sockaddr_in net_socket_addr;
    net_socket_addr.sin_family = AF_INET;
    net_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    net_socket_addr.sin_port = htons(port_num);

    if(bind(net_socket_fd, (struct sockaddr*) &net_socket_addr, sizeof(net_socket_addr)) != 0)
        raise_error("Error with binding net socket");
    
    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
        raise_error("Error with creating epoll");
    
    struct epoll_event e_event;
    memset(&e_event, 0, sizeof(struct epoll_event));
    e_event.events = EPOLLIN | EPOLLPRI;
    
    e_event.data.fd = unix_socket_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket_fd, &e_event) != 0)
        raise_error("Error with adding UNIX socket to epoll");
    
    e_event.data.fd = net_socket_fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, net_socket_fd, &e_event) != 0)
        raise_error("Error with adding net socket to epoll");
}

int check_name_occurence(const char* name)
{
    for(int i  = 0; i < current_clients; i++)
    {
        if(strcmp(clients[i].name, name) == 0)
            return i;
    }

    return -1;
}

void add_new_client(int socket_fd, struct Message msg, struct sockaddr client_addr, socklen_t socklen)
{
    uint8_t msg_type;

    if(current_clients == MAX_CLIENT_CONNECTIONS)
    {
        msg_type = CLIENT_OVERFLOW;        
    }
    else if(check_name_occurence(msg.client_name) != -1)
    {
        msg_type = WRONG_NAME;
    }
    else
    {
        msg_type = ADDED_CLIENT;

        clients[current_clients].is_busy = 0;
        clients[current_clients].is_pinged = 0;
        clients[current_clients].socklen = socklen;
        clients[current_clients].mode = msg.mode;
        clients[current_clients].name = malloc((strlen(msg.client_name) + 1) * sizeof(char));        
        if(clients[current_clients].name == NULL)
            raise_error("Error with memory allocation for client name");
        
        strcpy(clients[current_clients].name, msg.client_name);

        clients[current_clients].client_addr = malloc(sizeof(struct sockaddr));
        if(clients[current_clients].client_addr == NULL)
            raise_error("Error with memory allocation for client address");
        memcpy(clients[current_clients].client_addr, &client_addr, socklen);        

        current_clients++;
        printf("New client added: %s\n", msg.client_name);
    }

    if(sendto(socket_fd, &msg_type, sizeof(uint8_t), 0, &client_addr, socklen) != sizeof(uint8_t))
        raise_error("Error with sending message to client");        
}

void delete_client(int index)
{
    if(index == -1)
    {  
        errno = EINVAL;
        raise_error("Trying to delete non existing client");
    }

    printf("Client %s deleted\n", clients[index].name);
    free(clients[index].name);
    free(clients[index].client_addr);
    current_clients--;

    for(int i = index; i < current_clients; i++)
    {
        clients[i] = clients[i + 1];
    }
}

void read_message(int socket_fd)
{
    struct sockaddr client_addr;
    socklen_t socklen = sizeof(struct sockaddr);
    struct Message msg; 
    int client_index = -1;

    if(recvfrom(socket_fd, &msg, sizeof(struct Message), 0, &client_addr, &socklen) != sizeof(struct Message))
        raise_error("Error with reading new message");  

    switch(msg.type)
    {
        case NEW_CLIENT:            
            add_new_client(socket_fd, msg, client_addr, socklen);
            break;
        
        case DELETE_CLIENT:            
            delete_client(check_name_occurence(msg.client_name));
            break;

        case PING_CLIENT:            
            client_index = check_name_occurence(msg.client_name);
            if(client_index == -1)
            {
                errno = EINVAL;
                raise_error("Wrong client ID read");
            }
            
            clients[client_index].is_pinged = 0;
            break;

        case TASK_RESULT:           
            client_index = check_name_occurence(msg.client_name);
            if(client_index == -1)
            {
                errno = EINVAL;
                raise_error("Wrong client ID read");
            }

            clients[client_index].is_busy -= 1;
            printf("\nTask ID %d made by %s finished\nResult:\n%s\n", msg.mode, msg.client_name, msg.message_text);
            break;

        default:
            errno = EINVAL;
            raise_error("Unrecognized message type");
            break;
    }
}

void* epoll_routine(void* arg)
{
    struct epoll_event caught_event;

    while(server_working == 1)
    {
        if(epoll_wait(epoll_fd, &caught_event, 1, -1) == -1)
            raise_error("Error with reading epoll event");
        
        read_message(caught_event.data.fd);
    }

    pthread_exit(arg);
}

int first_non_busy_client(void)
{
    for(int i = 0; i < MAX_CLIENT_CONNECTIONS; i++)
    {
        if(clients[i].is_busy == 0)
            return i;
    }

    return -1;
}

void* terminal_routine(void* arg)
{
    char buffer[MAX_FILE_NAME_LENGTH];

    while(server_working == 1)
    {
        printf("\nType file to analyze path: ");
        scanf("%s", buffer);
        if(strlen(buffer) != 0)
        {
            int client = first_non_busy_client();
            if(client == -1)
                client = rand() % current_clients;

            clients[client].is_busy += 1;

            uint8_t msg_type = TASK;

            int client_socket = (clients[client].mode == UNIX) ? unix_socket_fd : net_socket_fd;

            if(sendto(client_socket, &msg_type, sizeof(uint8_t), 0, clients[client].client_addr, clients[client].socklen) != sizeof(uint8_t))
                raise_error("Error with sending message to client");     

            id++;
            struct Task sent_task;
            sent_task.id = id;
            strcpy(sent_task.file_path, buffer);   

            if(sendto(client_socket, &sent_task, sizeof(struct Task), 0, clients[client].client_addr, clients[client].socklen) != sizeof(struct Task))
                raise_error("Error with sending task to client");
        }
    }

    pthread_exit(arg);
}

void* ping_routine(void* arg)
{
    uint8_t msg_type = PING_SERVER;

    while(server_working == 1)
    {
        for(int i = 0; i < current_clients; i++)
        {
            if(clients[i].is_pinged == 1)
            {
                printf("Client %s not responding. Executing termination...\n", clients[i].name);
                delete_client(i);
            }
            else
            {
                int used_socket = clients[i].mode == UNIX ? unix_socket_fd : net_socket_fd;
                if(sendto(used_socket, &msg_type, sizeof(uint8_t), 0, clients[i].client_addr, clients[i].socklen) != sizeof(uint8_t))
                    raise_error("Error with sending ping message to client");
                clients[i].is_pinged = 1;
            }            
        }

        sleep(PING_FREQUENCY);
    }

    pthread_exit(arg);
}

void run_threads(void)
{
    if(pthread_create(&epoll_thread, NULL, &epoll_routine, NULL) != 0)
        raise_error("Error with creating epoll thread");
    
    if(pthread_create(&terminal_thread, NULL, &terminal_routine, NULL) != 0)
        raise_error("Error with creating terminal thread");

    if(pthread_create(&ping_thread, NULL, &ping_routine, NULL) != 0)
        raise_error("Error with creating ping thread");
}

int main(int argc, char* argv[])
{
    set_exit_handlers();
    parse_initial_arguments(argc, argv);
    initialize_sockets();
    run_threads();

    pthread_join(ping_thread, NULL);
    pthread_join(terminal_thread, NULL);
    pthread_join(epoll_thread, NULL);

    while(1)
    {
        sleep(10);
    }

    return 0;
}