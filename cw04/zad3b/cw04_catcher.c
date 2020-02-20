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

sigset_t waiting_mask, program_mask;

enum sending_mode {kill_mode, sigqueue_mode, sigrt_mode} mode;

void send_kill(void)
{
    kill(sender_pid, SIGUSR1);
    sigsuspend(&waiting_mask);
}

void send_sigqueue(void)
{
    union sigval sent_val;
    sent_val.sival_int = received_sigusr1;
    sigqueue(sender_pid, SIGUSR1, sent_val);
    sigsuspend(&waiting_mask);
}

void send_sigrt(void)
{
    kill(sender_pid, SIGINT);
    sigsuspend(&waiting_mask);
}

void send_sigusr2(void)
{
    switch (mode)
    {
        case kill_mode:
        {
            kill(sender_pid, SIGUSR2);
            break;
        }
        case sigqueue_mode:
        {
            union sigval sent_val;
            sent_val.sival_int = received_sigusr1;
            sigqueue(sender_pid, SIGUSR2, sent_val); 
            break;
        }       
        default:
            break;
    }
}

void send_sigtstp(void)
{
    kill(sender_pid, SIGTSTP);
}

void take_sigusr1(int signum, siginfo_t* info, void* context)
{
    if(sender_pid == 0)
        sender_pid = info->si_pid;
    
    received_sigusr1++;

    if(mode == kill_mode)
    {
        send_kill();
    }
    else if(mode == sigqueue_mode)
    {
        send_sigqueue();
    }
}

void take_sigint(int signum, siginfo_t* info, void* context)
{
    sender_pid = info->si_pid;
    received_sigusr1++;
    send_sigrt();
}

void take_sigusr2(int signum, siginfo_t* info, void* context)
{
    sender_pid = info->si_pid;
    printf("[Catcher]: %d SIGUSR1 signals received\n", received_sigusr1);
    send_sigusr2();
    exit(0);
}

void take_sigtstp(int signum, siginfo_t* info, void* context)
{
    sender_pid = info->si_pid;
    printf("[Catcher]: %d SIGINT signals received\n", received_sigusr1);
    send_sigtstp();
    exit(0);
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
    
    sigfillset(&program_mask);
    sigfillset(&waiting_mask);

    struct sigaction sigusr1_sender;
    struct sigaction sigusr2_sender;

    sigemptyset(&sigusr1_sender.sa_mask);
    sigusr1_sender.sa_flags = SA_SIGINFO;
    sigemptyset(&sigusr2_sender.sa_mask);
    sigusr2_sender.sa_flags = SA_SIGINFO;    

    switch (mode)
    {
        case kill_mode:
        case sigqueue_mode:           
            sigdelset(&program_mask, SIGUSR2);
            sigdelset(&waiting_mask, SIGUSR2);

            sigdelset(&waiting_mask, SIGUSR1);             
            
            sigusr1_sender.sa_sigaction = take_sigusr1;
            sigaction(SIGUSR1, &sigusr1_sender, NULL);
            
            sigusr2_sender.sa_sigaction = take_sigusr2;            
            sigaction(SIGUSR2, &sigusr2_sender, NULL);

            break;
        case sigrt_mode:
        {
            sigdelset(&program_mask, SIGTSTP);
            sigdelset(&waiting_mask, SIGTSTP);

            sigdelset(&waiting_mask, SIGINT); 

            sigusr1_sender.sa_sigaction = take_sigint;            
            sigaction(SIGINT, &sigusr1_sender, NULL);
            
            sigusr2_sender.sa_sigaction = take_sigtstp;
            sigaction(SIGTSTP, &sigusr2_sender, NULL);

            break;
        }
        default:
            break;
    }

    if(sigprocmask(SIG_SETMASK, &program_mask, NULL) != 0)
                printf("Error with creating signals mask!\n");     

    sigsuspend(&waiting_mask);

    return 0;
}