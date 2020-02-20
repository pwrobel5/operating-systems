#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#define EXPECTED_NUMBER_OF_ARGUMENTS 4
#define CATCHER_PID_POSITION 1
#define NUMBER_OF_SIGNALS_POSITION 2
#define MODE_POSITION 3

int received_sigusr1 = 0;
int number_of_signals = 0;

enum sending_mode {kill_mode, sigqueue_mode, sigrt_mode} mode;

int parse_initial_arguments(int argc, char* argv[], pid_t* catcher_pid)
{
    if(argc != EXPECTED_NUMBER_OF_ARGUMENTS)
    {
        printf("Wrong arguments number!\n");
        return 1;
    }

    *catcher_pid = (pid_t) atoi(argv[CATCHER_PID_POSITION]);
    if(*catcher_pid == 0)
    {
        printf("Wrong catcher PID!\n");
        return 1;
    }

    number_of_signals = atoi(argv[NUMBER_OF_SIGNALS_POSITION]);
    if(number_of_signals == 0)
    {
        printf("Wrong number of signals!\n");
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

void send_by_kill(pid_t catcher_pid)
{
    for(int i = 0; i < number_of_signals; i++)
    {
        kill(catcher_pid, SIGUSR1);
    }

    kill(catcher_pid, SIGUSR2);
}

void send_by_sigqueue(pid_t catcher_pid)
{
    union sigval sent_val;
    for(int i = 0; i < number_of_signals; i++)
    {
        sent_val.sival_int = i;
        sigqueue(catcher_pid, SIGUSR1, sent_val);
    }
    sigqueue(catcher_pid, SIGUSR2, sent_val);
}

void send_by_sigrt(pid_t catcher_pid)
{
    for(int i = 0; i < number_of_signals; i++)
    {
        kill(catcher_pid, SIGINT);
    }

    kill(catcher_pid, SIGTSTP);
}

void take_sigusr1_kill(int signum)
{
    received_sigusr1++;
}

void take_sigusr2(int signum)
{
    printf("SIGUSR1s sent: %d\n", number_of_signals);
    printf("SIGUSR1s received: %d\n", received_sigusr1);
    exit(0);
}

void take_sigusr1_sigqueue(int signum, siginfo_t* info, void* context)
{
    received_sigusr1++;
    printf("[%d] Received SIGUSR1 number %d from catcher\n", received_sigusr1, info->si_int);
}

void take_sigtstp(int signum)
{
    printf("SIGINTs sent: %d\n", number_of_signals);
    printf("SIGINTs received: %d\n", received_sigusr1);
    exit(0);
}

void take_sigint(int signum)
{
    received_sigusr1++;
}

int main(int argc, char* argv[])
{
    pid_t catcher_pid;

    if(parse_initial_arguments(argc, argv, &catcher_pid) != 0)
        return 1;   
    
    switch(mode)
    {
        case kill_mode:
        {
            struct sigaction sigusr1_sender_kill;
            sigusr1_sender_kill.sa_handler = take_sigusr1_kill;
            sigemptyset(&sigusr1_sender_kill.sa_mask);
            sigusr1_sender_kill.sa_flags = 0;
            sigaction(SIGUSR1, &sigusr1_sender_kill, NULL);

            struct sigaction sigusr2_sender_kill;
            sigusr2_sender_kill.sa_handler = take_sigusr2;
            sigemptyset(&sigusr2_sender_kill.sa_mask);
            sigusr2_sender_kill.sa_flags = 0;
            sigaction(SIGUSR2, &sigusr2_sender_kill, NULL);

            send_by_kill(catcher_pid);
            break;
        }
            
        case sigqueue_mode:
        {
            struct sigaction sigusr1_sender_sigqueue;
            sigusr1_sender_sigqueue.sa_sigaction = take_sigusr1_sigqueue;
            sigemptyset(&sigusr1_sender_sigqueue.sa_mask);
            sigusr1_sender_sigqueue.sa_flags = SA_SIGINFO;
            sigaction(SIGUSR1, &sigusr1_sender_sigqueue, NULL);

            struct sigaction sigusr2_sender_sigqueue;
            sigusr2_sender_sigqueue.sa_handler = take_sigusr2;
            sigemptyset(&sigusr2_sender_sigqueue.sa_mask);
            sigusr2_sender_sigqueue.sa_flags = 0;
            sigaction(SIGUSR2, &sigusr2_sender_sigqueue, NULL);

            send_by_sigqueue(catcher_pid);
            break;
        }            
        case sigrt_mode:
        {
            struct sigaction sigint_sigrt;
            sigint_sigrt.sa_handler = take_sigint;
            sigemptyset(&sigint_sigrt.sa_mask);
            sigint_sigrt.sa_flags = 0;
            sigaction(SIGINT, &sigint_sigrt, NULL);

            struct sigaction sigtstp_sigrt;
            sigtstp_sigrt.sa_handler = take_sigtstp;
            sigemptyset(&sigtstp_sigrt.sa_mask);
            sigtstp_sigrt.sa_flags = 0;
            sigaction(SIGTSTP, &sigtstp_sigrt, NULL);

            send_by_sigrt(catcher_pid);
            break;
        }
        default:
            break;
    }

    while(1)
    {
        pause();
    }

    return 0;
}