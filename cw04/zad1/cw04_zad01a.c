#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

int program_status = 1;

void take_sigint(int signum)
{
    printf("Odebrano sygnal SIGINT\n");
    exit(0);
}

void take_sigtstp(int signum)
{
    program_status = 1 - program_status;
}

int main()
{
    signal(SIGINT, take_sigint);

    struct sigaction act;
    act.sa_handler = take_sigtstp;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGTSTP, &act, NULL);

    while(1)
    {
        if(program_status == 0)
        {
            printf("Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");
            pause();
        }

        system("date");
        sleep(1);
    }
}