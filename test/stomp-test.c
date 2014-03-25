#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/CUnit.h>

#include "stomp.h"

void test_command_connect() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0, parse_command("CONNECT\nlogin: client-1\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("CONNECT", cmd.name);
    CU_ASSERT_STRING_EQUAL("login", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("client-0", cmd.headers->value);
    CU_ASSERT_PTR_NULL(cmd.content);

    // missing login header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER, parse_command("CONNECT\n\n\0", &cmd));

    // no value for login header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER, parse_command("CONNECT\nlogin: \0", &cmd));
 
    // command expects no body
    CU_ASSERT_EQUAL(STOMP_UNEXPECTED_BODY, parse_command("CONNECT\nlogin: client-1\n\nhello\n\0", &cmd));
}

// TODO connected

void test_command_send() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0, parse_command("SEND\ntopic: stocks\n\nnew price: 33.4\n\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_STRING_EQUAL("new price: 33.4", cmd.content);
 
    // multiline regular command
    CU_ASSERT_EQUAL(0, parse_command("SEND\ntopic: stocks\n\nold price: 23.4\nnew price: 33.4\n\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_STRING_EQUAL("old price: 23.4\nnew price: 33.4", cmd.content);

    // missing topic header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER, parse_command("SEND\n\nnew price: 33.4\n\0", &cmd));

    // no value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER, parse_command("SEND\ntopic:\n\n new price: 33.4\n\n\0", &cmd));
    // empty value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER, parse_command("SEND\ntopic: \n\n new price: 33.4\n\n\0", &cmd));
}

void test_command_subscribe() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0, parse_command("SUBSCRIBE\ndestination: stocks\n\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("SUBSCRIBE", cmd.name);
    CU_ASSERT_STRING_EQUAL("destination", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_PTR_NULL(cmd.content);
 
    // missing topic header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER, parse_command("SUBSCRIBE\n\n\0", &cmd));

    // no value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER, parse_command("SUBSCRIBE\ndestination:\n\n\0", &cmd));
    // empty value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER, parse_command("SUBSCRIBE\ndestination: \n\n\0", &cmd));
 
    // command expects no body
    CU_ASSERT_EQUAL(STOMP_UNEXPECTED_BODY, parse_command("SUBSCRIBE\ndestination: stocks\n\nhello\n\n\0", &cmd));
}

// function that takes an error value and
// returns a frame ready to sent to a client

int main(int argc, char** argv) {
    
    exit(EXIT_SUCCESS);
}
