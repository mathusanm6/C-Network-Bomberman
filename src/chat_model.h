#ifndef CHAT_MODEL_H
#define CHAT_MODEL_H

#include "constants.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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

chat *create_chat();

void free_chat(chat *c);

/** Decrements the line cursor
 */
void decrement_line(chat *c);

/** Nullifies the line cursor
 */
void clear_line(chat *c);

/** Adds the character at the end of chat_line if it does not exceed TEXT_SIZE and increment the cursor
 */
void add_to_line(chat *chat, char c);

/** Adds a message to the chat history sent by the server
 */
int add_message_from_server(chat *c, int sender, char *message, bool whispered);

/** Adds a message to the chat history sent by the client
 */
int add_message_from_client(chat *c, int client_id, char **message, bool *whispered);

bool is_chat_on_focus(chat *c);

void set_chat_focus(chat *c, bool on_focus);

/** Toggles the whispering flag in the chat
 */
void toggle_whispering(chat *c);

#endif // CHAT_MODEL_H_