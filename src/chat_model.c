#include "./chat_model.h"

#define EMPTY_CHAR '\0'

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./utils.h"

chat *chat_ = NULL;

int init_chat() {
    if (chat_ == NULL) {
        chat_ = malloc(sizeof(chat));
        RETURN_FAILURE_IF_NULL_PERROR(chat_, "malloc");

        chat_->history = malloc(sizeof(chat_history));
        RETURN_FAILURE_IF_NULL_PERROR(chat_->history, "malloc");
        chat_->history->count = 0;

        chat_->line = malloc(sizeof(chat_line));
        RETURN_FAILURE_IF_NULL_PERROR(chat_->line, "malloc");
        chat_->line->cursor = 0;
        chat_->on_focus = false;
        chat_->whispering = false;
    }
    return EXIT_SUCCESS;
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

void free_chat() {
    if (chat_ == NULL) {
        return;
    }

    if (chat_->history != NULL) {
        free_chat_history(chat_->history);
        chat_->history = NULL;
    }

    if (chat_->line != NULL) {
        free(chat_->line);
        chat_->line = NULL;
    }

    free(chat_);
    chat_ = NULL;
}

void decrement_line() {
    if (chat_->line != NULL && chat_->line->cursor > 0) {
        chat_->line->cursor--;
        chat_->line->data[chat_->line->cursor] = EMPTY_CHAR;
    }
}

void clear_line() {
    if (chat_->line != NULL) {
        memset(chat_->line->data, EMPTY_CHAR, chat_->line->cursor);
        chat_->line->cursor = 0;
    }
}

void add_to_line(char c) {
    if (chat_->line != NULL && chat_->line->cursor < TEXT_SIZE && c >= ' ' && c <= '~') {
        chat_->line->data[(chat_->line->cursor)] = c;
        (chat_->line->cursor)++;
    }
}

chat_node *create_chat_node(int sender, char msg[TEXT_SIZE], bool whispered) {
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

void add_message(int sender, char *msg, bool whispered) {
    if (chat_->history == NULL) {
        return;
    }

    chat_node *new_node = create_chat_node(sender, msg, whispered);
    if (new_node == NULL) {
        return;
    }

    if (chat_->history->count == MAX_CHAT_HISTORY_LEN) {
        // If the history is full, replace the oldest message
        chat_node *temp = chat_->history->head;
        while (temp->next != chat_->history->head) { // Find the last node before head
            temp = temp->next;
        }
        temp->next = new_node;                       // Link the new node after the last node
        new_node->next = chat_->history->head->next; // New node points to second oldest node
        free(chat_->history->head);
        chat_->history->head = new_node->next; // New head is the second oldest node
    } else {
        // List is not full, add the new node to the end
        if (chat_->history->head == NULL) {
            chat_->history->head = new_node;
            new_node->next = new_node;
        } else {
            chat_node *temp = chat_->history->head;
            while (temp->next != chat_->history->head) {
                temp = temp->next;
            }
            temp->next = new_node;
            new_node->next = chat_->history->head;
        }
        chat_->history->count++;
    }
}

void toggle_whispering() {
    chat_->whispering = !chat_->whispering;
}