#ifndef CW10_STREAM_HEADER
#define CW10_STREAM_HEADER

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

void raise_error(const char* message);
void send_message(int fd_to, uint8_t type, uint16_t size, void* text);

#endif