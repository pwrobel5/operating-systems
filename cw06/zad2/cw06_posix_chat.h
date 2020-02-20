#ifndef CW06_POSIX_CHAT
#define CW06_POSIX_CHAT

#define SERVER_QUEUE_NAME "/cw06serverqueue"
#define QUEUE_PERMISSIONS 0666

#define MAX_MSG_SIZE 500
#define MAX_MSG_NUM 8
#define MAX_CLIENTS_NUMBER 100
#define QUEUE_NAME_MAX_LEN 40
#define BLANK_FRIENDS_STR "-1"

struct node {
    int friend_id;
    struct node* next;
};

enum message_type {
    STOP = 1L,
    LIST = 2L,
    FRIENDS = 3L,
    ECHO = 4L,
    ADD = 5L,
    DEL = 6L,
    _2ALL = 7L,
    _2FRIENDS = 8L,
    _2ONE = 9L,
    INIT = 10L,
};

unsigned int priority(enum message_type msg_type)
{
    switch(msg_type)
    {
        case STOP:
            return 7;
        case LIST:
            return 6;
        case FRIENDS:
            return 5;
        case _2ALL:
            return 4;
        case _2FRIENDS:
            return 3;
        case _2ONE:
            return 2;
        default:
            return 1;
    }
}

#endif