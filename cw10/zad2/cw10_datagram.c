#include <stdio.h>
#include <stdlib.h>

#include "cw10_datagram.h"

void raise_error(const char* message)
{
    perror(message);
    exit(EXIT_FAILURE);
}