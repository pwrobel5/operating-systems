#ifndef CW10_DATAGRAM_HEADER
#define CW10_DATAGRAM_HEADER

#define TYPE_LENGTH 1
#define SIZE_LENGTH 2

#define MAX_FILE_NAME_LENGTH 50
#define MAX_FILE_SIZE 16384
#define MAX_COMMAND_SIZE 150
#define MAX_AUTHOR_NAME 20

enum Connection_type {
    UNIX, NET
};

enum Message_type {
    NEW_CLIENT, ADDED_CLIENT, CLIENT_OVERFLOW, WRONG_NAME, PING_SERVER, PING_CLIENT, DELETE_CLIENT, TASK, TASK_RESULT, SERVER_DISCONNECT
};

struct Task {
    int id;
    char file_path[MAX_FILE_NAME_LENGTH];
};

struct Task_res {
    int id;
    char author_name[MAX_AUTHOR_NAME];
    char result[MAX_FILE_SIZE];
};

struct Message {
    enum Message_type type;
    char client_name[MAX_AUTHOR_NAME];
    enum Connection_type mode;
    char message_text[MAX_FILE_SIZE];
};

void raise_error(const char* message);

#endif