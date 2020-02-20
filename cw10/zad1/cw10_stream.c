#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <arpa/inet.h>

#include "cw10_stream.h"

void raise_error(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

void send_message(int fd_to, uint8_t type, uint16_t size, void* text)
{
    if(write(fd_to, &type, TYPE_LENGTH) != TYPE_LENGTH)
        raise_error("Error with sending message type to client");
    
    if(write(fd_to, &size, SIZE_LENGTH) != SIZE_LENGTH)
        raise_error("Error with sending message size to client");
    
    if(write(fd_to, text, size) != size)
        raise_error("Error with sending message text to client");
}