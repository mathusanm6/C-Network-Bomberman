#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/messages.h"
#include "test.h"

void test_empty_message(test_info *info);
void test_short_message(test_info *info);
void test_2_to_254_long_message(test_info *info);
void test_255_long_message(test_info *info);
void test_256_long_message(test_info *info);
void test_too_long_message(test_info *info);
void test_team_message_invalid_type(test_info *info);
void test_team_message_invalid_id(test_info *info);
void test_team_message_invalid_eq(test_info *info);
void test_team_message_ignore_eq_on_global(test_info *info);

#define NUMBER_TESTS 10

test_info *serialization_chat() {
    test_case cases[NUMBER_TESTS] = {
        QUICK_CASE("Empty message", test_empty_message),
        QUICK_CASE("Short message", test_short_message),
        QUICK_CASE("Message of sizes between 2 and 254", test_2_to_254_long_message),
        QUICK_CASE("Message of size 255", test_255_long_message),
        QUICK_CASE("Message of size 256", test_256_long_message),
        QUICK_CASE("Too long message", test_too_long_message),
        QUICK_CASE("Team message invalid type", test_team_message_invalid_type),
        QUICK_CASE("Team message invalid id", test_team_message_invalid_id),
        QUICK_CASE("Team message invalid eq", test_team_message_invalid_eq),
        QUICK_CASE("Team message ignore eq on global", test_team_message_ignore_eq_on_global),
    };

    return cinta_run_cases("Serialization tests | Chat", cases, NUMBER_TESTS);
}

/** Assumes `message` to be `\0` terminated and `length` to be the length of the message without the `\0` */
void test_message(test_info *info, char *message, uint8_t length, chat_message_type type) {
    chat_message *msg = malloc(sizeof(chat_message));
    msg->message = message;
    msg->message_length = length;
    msg->type = type;
    for (int i = 0; i < 4; i++) {
        msg->id = i;

        for (int j = 0; j < 2; j++) {
            msg->eq = j;

            char *serialized = serialize_chat_message(msg);
            chat_message *deserialized = deserialize_chat_message(serialized);

            CINTA_ASSERT_INT(deserialized->type, msg->type, info);
            CINTA_ASSERT_INT(deserialized->id, msg->id, info);
            CINTA_ASSERT_INT(deserialized->eq, msg->eq, info);
            CINTA_ASSERT_INT(deserialized->message_length, msg->message_length, info);
            CINTA_ASSERT(strncmp(deserialized->message, msg->message, length) == 0, info);

            free(serialized);
            free(deserialized->message);
            free(deserialized);
        }
    }

    free(msg);
}

void test_empty_message(test_info *info) {
    test_message(info, "", 0, GLOBAL_M);
    test_message(info, "", 0, TEAM_M);
}

void test_short_message(test_info *info) {
    test_message(info, "a", 1, GLOBAL_M);
    test_message(info, "a", 1, TEAM_M);
}

void test_2_to_254_long_message(test_info *info) {
    for (int i = 2; i < 255; i++) {
        char *message = malloc(i);
        memset(message, 'a', i);
        test_message(info, message, i, GLOBAL_M);
        test_message(info, message, i, TEAM_M);
        free(message);
    }
}

void test_255_long_message(test_info *info) {
    char *message = malloc(255);
    memset(message, 'a', 255);
    test_message(info, message, 255, GLOBAL_M);
    test_message(info, message, 255, TEAM_M);
    free(message);
}

void test_256_long_message(test_info *info) {
    char *message = malloc(256);
    memset(message, 'a', 256);
    // 255 as size is the maximum size of a message
    test_message(info, message, 255, GLOBAL_M);
    test_message(info, message, 255, TEAM_M);
    free(message);
}

void test_too_long_message(test_info *info) {
    // Namarie ! (Sorry for the quenya, I cannot resist. Also sorry for removing the punctuation, feels painful)
    char *message = "Ai! laurie lantar lassi surinen, yeni unotime ve ramar aldaron! Yeni ve linte yuldar avanier mi "
                    "oromardi lisse-miruvoreva Andune pella, Vardo tellumar nu luini yassen tintilar i eleni omaryo "
                    "airetari-lirinen. .....ar hisie untupa Calaciryo miri oiale. Si vanwa na, Romello vanwa, Valimar! "
                    "Namarie! Nai hiruvalye Valimar! Nai elye hiruva! Namarie!";
    test_message(info, message, 255, GLOBAL_M);
    test_message(info, message, 255, TEAM_M);
}

void test_team_message_invalid_type(test_info *info) {
    chat_message *msg = malloc(sizeof(chat_message));
    msg->message = "Hello";
    msg->message_length = 5;
    msg->type = GLOBAL_M + 10;
    msg->id = 0;
    msg->eq = 0;

    char *serialized = serialize_chat_message(msg);
    CINTA_ASSERT_NULL(serialized, info);

    free(msg);
}

void test_team_message_invalid_id(test_info *info) {
    chat_message *msg = malloc(sizeof(chat_message));
    msg->message = "Hello";
    msg->message_length = 5;
    msg->type = TEAM_M;
    msg->id = 5;
    msg->eq = 0;

    char *serialized = serialize_chat_message(msg);
    CINTA_ASSERT_NULL(serialized, info);

    free(msg);
}

void test_team_message_invalid_eq(test_info *info) {
    chat_message *msg = malloc(sizeof(chat_message));
    msg->message = "Hello";
    msg->message_length = 5;
    msg->type = TEAM_M;
    msg->id = 0;
    msg->eq = 2;

    char *serialized = serialize_chat_message(msg);
    CINTA_ASSERT_NULL(serialized, info);

    free(msg);
}

void test_team_message_ignore_eq_on_global(test_info *info) {
    chat_message *msg = malloc(sizeof(chat_message));
    msg->message = "Hello";
    msg->message_length = 5;
    msg->type = GLOBAL_M;
    msg->id = 0;
    msg->eq = 3;

    char *serialized = serialize_chat_message(msg);
    chat_message *deserialized = deserialize_chat_message(serialized);

    CINTA_ASSERT_INT(deserialized->type, msg->type, info);
    CINTA_ASSERT_INT(deserialized->id, msg->id, info);
    CINTA_ASSERT_INT(deserialized->eq, 1, info);
    CINTA_ASSERT_INT(deserialized->message_length, msg->message_length, info);
    CINTA_ASSERT_STRING(deserialized->message, msg->message, info);

    free(msg);
    free(serialized);
    free(deserialized->message);
    free(deserialized);
}
