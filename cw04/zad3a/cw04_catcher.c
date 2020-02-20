#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define MODE_POSITION 1
#define EXPECTED_NUMBER_OF_ARGUMENTS 2

pid_t sender_pid = 0;
int received_sigusr1 = 0;

enum sending_mode {kill_mode, sigqueue_mode, sigrt_mode} mode;

void send_signals()
{
    switch(mode)
    {
        case kill_mode:
        {
            for(int i = 0; i < received_sigusr1; i++)
            {
                kill(sender_pid, SIGUSR1);
            }

            kill(sender_pid, SIGUSR2);
            exit(0);
            break;
        }
        case sigqueue_mode:
        {
            union sigval sent_val;
            for(int i = 0; i < received_sigusr1; i++)
            {
                
                sent_val.sival_int = i;
                sigqueue(sender_pid, SIGUSR1, sent_val);
            }

            sigqueue(sender_pid, SIGUSR2, sent_val);
            exit(0);
            break;
        }
        case sigrt_mode:
        {
            for(int i = 0; i < received_sigusr1; i++)
            {
                kill(sender_pid, SIGINT);
            }

            kill(sender_pid, SIGTSTP);
            exit(0);
            break;
        }
    }
    
}

void take_sigusr1(int signum)
{
    received_sigusr1++;
}

void take_sigint(int signum)
{
    received_sigusr1++;
}

void take_sigusr2(int signum, siginfo_t* info, void* context)
{
    sender_pid = info->si_pid;
    send_signals();
}

void take_sigtstp(int signum, siginfo_t* info, void* context)
{
    sender_pid = info->si_pid;
    send_signals();
}

int parse_initial_arguments(int argc, char* argv[])
{
    if(argc != EXPECTED_NUMBER_OF_ARGUMENTS)
    {
        printf("Wrong arguments number!\n");
        return 1;
    }

    if(strcmp(argv[MODE_POSITION], "kill_m") == 0)
        mode = kill_mode;
    else if(strcmp(argv[MODE_POSITION], "sigqueue_m") == 0)
        mode = sigqueue_mode;
    else if(strcmp(argv[MODE_POSITION], "sigrt_m") == 0)
        mode = sigrt_mode;
    else
    {
        printf("Wrong mode!\n");
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    printf("Catcher PID: %d\n", getpid());

    if(parse_initial_arguments(argc, argv) != 0)
        return 1;
    
    sigset_t blocked_signals;
    struct sigaction sigusr1_sender;
    struct sigaction sigusr2_sender;

    sigfillset(&blocked_signals);
    sigemptyset(&sigusr1_sender.sa_mask);
    sigusr1_sender.sa_flags = 0;
    sigemptyset(&sigusr2_sender.sa_mask);
    sigusr2_sender.sa_flags = SA_SIGINFO;    

    switch (mode)
    {
        case kill_mode:
        case sigqueue_mode:
        {            
            sigdelset(&blocked_signals, SIGUSR1);
            sigdelset(&blocked_signals, SIGUSR2);             
            
            sigusr1_sender.sa_handler = take_sigusr1;
            sigaction(SIGUSR1, &sigusr1_sender, NULL);
            
            sigusr2_sender.sa_sigaction = take_sigusr2;            
            sigaction(SIGUSR2, &sigusr2_sender, NULL);

            break;
        }
        case sigrt_mode:
        {
            sigdelset(&blocked_signals, SIGINT);
            sigdelset(&blocked_signals, SIGTSTP);

            sigusr1_sender.sa_handler = take_sigint;            
            sigaction(SIGINT, &sigusr1_sender, NULL);
            
            sigusr2_sender.sa_sigaction = take_sigtstp;
            sigaction(SIGTSTP, &sigusr2_sender, NULL);

            break;
        }
        default:
            break;
    }

    if(sigprocmask(SIG_SETMASK, &blocked_signals, NULL) != 0)
                printf("Error with creating signals mask!\n");     

    while(1)
    {
        pause();
    }

    return 0;
}