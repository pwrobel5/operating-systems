#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>


int is_script_running = 1;
int is_script_process_dead = 1;

void take_sigint(int signum)
{
    printf("Odebrano sygnal SIGINT\n");
    exit(0);
}

void take_sigtstp(int signum)
{
    is_script_running = 1 - is_script_running;
}

int main()
{
    struct sigaction act;
    act.sa_handler = take_sigtstp;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    signal(SIGINT, take_sigint);
    sigaction(SIGTSTP, &act, NULL);

    pid_t child_pid;
    sigset_t child_proc;

    sigemptyset(&child_proc);
    sigaddset(&child_proc, SIGTSTP);

    while(1)
    {
        if(is_script_running == 0 && is_script_process_dead == 0)
        {
            printf("Oczekuje na CTRL+Z - kontynuacja albo CTRL+C - zakonczenie programu\n");
            kill(child_pid, SIGKILL);
            is_script_process_dead = 1;
            pause();
        }
        else if(is_script_running == 1 && is_script_process_dead == 1)
        {
            child_pid = fork();
            if(child_pid == 0)
            {
                printf("Uruchamiam nowy proces o PID: %d\n", (int) getpid());

                if(sigprocmask(SIG_SETMASK, &child_proc, NULL) < 0)
                {
                    printf("Nie udalo sie ustawic maski dla procesu potomnego!\n");
                }

                execlp("./date_script.sh", "./date_script.sh", NULL);
                exit(0);
            }

            is_script_process_dead = 0;
            pause();
        }        
    }
}