#include "chat_model.h"
#include "./utils.h"

#define EMPTY_CHAR '\0'

chat *create_chat() {
    chat *c = malloc(sizeof(chat));
    RETURN_NULL_IF_NULL_PERROR(c, "malloc");

    c->history = malloc(sizeof(chat_history));
    if (c->history == NULL) {
        free(c);
        return NULL;
    }
    c->history->count = 0;

    c->line = malloc(sizeof(chat_line));
    if (c->line == NULL) {
        free(c->history);
        free(c);
        return NULL;
    }
    c->line->cursor = 0;
    c->on_focus = false;
    c->whispering = false;

    return c;
}

void free_chat_node(chat_node *node) {
    if (node == NULL) {
        return;
    }

    free(node);
}

void free_chat_history(chat_history *history) {
    if (history == NULL) {
        return;
    }

    if (history->head == NULL) {
        free(history);
        return;
    }

    chat_node *current = history->head->next; // Start from the second node
    chat_node *next;

    while (current != history->head) {
        next = current->next;
        free_chat_node(current);
        current = next;
    }

    free_chat_node(history->head);

    history->head = NULL;
    history->count = 0;

    free(history);
}

void free_chat(chat *c) {
    if (c == NULL) {
        return;
    }

    if (c->history != NULL) {
        free_chat_history(c->history);
        c->history = NULL;
    }

    if (c->line != NULL) {
        free(c->line);
        c->line = NULL;
    }

    free(c);
}

void decrement_line(chat *c) {
    RETURN_IF_NULL(c);

    if (c->line != NULL && c->line->cursor > 0) {
        c->line->cursor--;
        c->line->data[c->line->cursor] = EMPTY_CHAR;
    }
}

void clear_line(chat *c) {
    RETURN_IF_NULL(c);

    if (c->line != NULL) {
        memset(c->line->data, EMPTY_CHAR, c->line->cursor);
        c->line->cursor = 0;
    }
}

void add_to_line(chat *c, char c) {
    RETURN_IF_NULL(c);

    if (c->line != NULL && c->line->cursor < TEXT_SIZE && c >= ' ' && c <= '~') {
        c->line->data[(c->line->cursor)] = c;
        (c->line->cursor)++;
    }
}

chat_node *create_chat_node(int sender, char msg[TEXT_SIZE], bool whispered) {
    if (sender < 0 || sender >= PLAYER_NUM) {
        return NULL;
    }

    RETURN_NULL_IF_NULL(msg);

    if (strlen(msg) == 0) {
        return NULL;
    }

    chat_node *new_node = malloc(sizeof(chat_node));
    if (new_node != NULL) {
        new_node->sender = sender;
        strncpy(new_node->message, msg, TEXT_SIZE);
        // Ensure null terminated string
        new_node->message[TEXT_SIZE - 1] = '\0';
        new_node->whispered = whispered;
        new_node->next = NULL;
    }
    return new_node;
}

int add_message_from_server(char *c, int sender, char *message, bool whispered) {
    RETURN_FAILURE_IF_NULL(c);
    RETURN_FAILURE_IF_NULL(c->history);

    chat_node *new_node = create_chat_node(sender, message, whispered);
    RETURN_FAILURE_IF_NULL(new_node);

    if (c->history->count == MAX_CHAT_HISTORY_LEN) {
        // If the history is full, replace the oldest message
        chat_node *temp = c->history->head;
        while (temp->next != c->history->head) { // Find the last node before head
            temp = temp->next;
        }
        temp->next = new_node;                   // Link the new node after the last node
        new_node->next = c->history->head->next; // New node points to second oldest node
        free(c->history->head);
        c->history->head = new_node->next; // New head is the second oldest node
    } else {
        // List is not full, add the new node to the end
        if (c->history->head == NULL) {
            c->history->head = new_node;
            new_node->next = new_node;
        } else {
            chat_node *temp = c->history->head;
            while (temp->next != c->history->head) {
                temp = temp->next;
            }
            temp->next = new_node;
            new_node->next = c->history->head;
        }
        c->history->count++;
    }

    return EXIT_SUCCESS;
}

int add_message_from_client(chat *c, int client_id, char **message, bool *whispered) {
    RETURN_FAILURE_IF_NULL(c);
    RETURN_FAILURE_IF_NULL(c->history);

    if (c->line->cursor == 0) {
        return EXIT_FAILURE;
    }

    chat_node *new_node = create_chat_node(client_id, c->line->data, c->whispering);
    RETURN_FAILURE_IF_NULL(new_node);

    // Pass the message outside when adding to history
    *message = malloc(sizeof(char) * (c->line->cursor + 1)); // +1 for null terminator
    RETURN_FAILURE_IF_NULL(*message);
    strncpy(*message, c->line->data, c->line->cursor);
    (*message)[c->line->cursor] = '\0'; // Null-terminate the string

    // Pass the whispering status outside
    *whispered = c->whispering;

    if (c->history->count == MAX_CHAT_HISTORY_LEN) {
        // If the history is full, replace the oldest message
        chat_node *temp = c->history->head;
        while (temp->next != c->history->head) { // Find the last node before head
            temp = temp->next;
        }
        temp->next = new_node;                   // Link the new node after the last node
        new_node->next = c->history->head->next; // New node points to second oldest node
        free(c->history->head);
        c->history->head = new_node->next; // New head is the second oldest node
    } else {
        // List is not full, add the new node to the end
        if (c->history->head == NULL) {
            c->history->head = new_node;
            new_node->next = new_node;
        } else {
            chat_node *temp = c->history->head;
            while (temp->next != c->history->head) {
                temp = temp->next;
            }
            temp->next = new_node;
            new_node->next = c->history->head;
        }
        c->history->count++;
    }

    return EXIT_SUCCESS;
}

bool is_chat_on_focus(chat *c) {
    if (c == NULL) {
        return false;
    }
    return c->on_focus;
}

void set_chat_focus(chat *c, bool on_focus) {
    RETURN_IF_NULL(c);
    c->on_focus = on_focus;
}

void toggle_whispering(chat *c) {
    RETURN_IF_NULL(c);
    c->whispering = !c->whispering;
}