#ifndef CW06_SYSTEM_V_CHAT
#define CW06_SYSTEM_V_CHAT

#define ENV_VAR_FOR_FTOK "HOME"
#define FTOK_LETTER 'u'
#define MSGGET_PERMISSIONS 0666

#define MAX_CLIENTS_NUMBER 100
#define MAX_MESSAGE_LENGTH 500
#define MSG_SIZE (sizeof(struct message) - sizeof(long))
#define TYPES_NUMBER 11

#define BLANK_FRIENDS_STR "-1"

struct node {
    int friend_id;
    struct node* next;
};

struct client_data {
    int queue_id;
    pid_t client_pid;
    struct node* friends_list;
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

struct message {
    long type;
    char msg[MAX_MESSAGE_LENGTH];
    pid_t client_id;
};

#endif