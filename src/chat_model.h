#ifndef SRC_CHAT_MODEL_H_
#define SRC_CHAT_MODEL_H_

#define TEXT_SIZE 60
#define MAX_CHAT_HISTORY_LEN 23

#include <stdbool.h>

typedef enum CHAT_ACTION {
    CHAT_WRITE = 0,
    CHAT_ERASE = 1,
    CHAT_SEND = 2,
    CHAT_TOGGLE_WHISPER = 3,
    CHAT_CLEAR = 4,
    CHAT_QUIT = 5,
    CHAT_GAME_QUIT = 6,
    CHAT_SWITCH_PLAYER = 7,
    CHAT_NONE = 8
} CHAT_ACTION;

typedef struct chat_node {
    int sender;
    char message[TEXT_SIZE];
    bool whispered;
    struct chat_node *next;
} chat_node;

typedef struct chat_history {
    chat_node *head;
    int count;
} chat_history;

typedef struct chat_line {
    char data[TEXT_SIZE];
    int cursor;
} chat_line;

typedef struct chat {
    chat_history *history;
    chat_line *line;
    bool on_focus;
    bool whispering;
} chat;

extern chat *chat_;

int init_chat();

void free_chat();

/** Decrements the line cursor
 */
void decrement_line();

/** Nullifies the line cursor
 */
void clear_line();

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(char);

/** Adds a message to the chat history with the sender and the message content and sets the whispered flag
 */
void add_message(int sender, char *msg, bool whispered);

/** Toggles the whispering flag in the chat
 */
void toggle_whispering();

#endif // SRC_CHAT_MODEL_H_