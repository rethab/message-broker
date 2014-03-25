#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../src/stomp.h"

void test_command_connect() {
    struct stomp_command cmd;

    // regular command
    assert(0 == parse_command("CONNECT\nlogin: client-1\n\0", &cmd));
    assert(0 == strcmp("CONNECT", cmd.name));
    assert(0 == strcmp("login", cmd.headers->key));
    assert(0 == strcmp("client-0", cmd.headers->value));
    assert(NULL == cmd.content);

    // missing login header
    assert(STOMP_MISSING_HEADER == parse_command("CONNECT\n\n\0", &cmd));

    // no value for login header
    assert(STOMP_INVALID_HEADER == parse_command("CONNECT\nlogin: \0", &cmd));
 
    // command expects no body
    assert(STOMP_UNEXPECTED_BODY
           == parse_command("CONNECT\nlogin: client-1\n\nhello\n\0", &cmd));
}

// TODO connected

void test_command_send() {
    struct stomp_command cmd;

    // regular command
    assert(0 ==
        parse_command("SEND\ntopic: stocks\n\nnew price: 33.4\n\n\0", &cmd));
    assert(0 == strcmp("SEND", cmd.name));
    assert(0 == strcmp("topic", cmd.headers->key));
    assert(0 == strcmp("stocks", cmd.headers->value));
    assert(0 == strcmp("new price: 33.4", cmd.content);
 
    // multiline regular command
    assert(0 == parse_command(
        "SEND\ntopic: stocks\n\nold price: 23.4\nnew price: 33.4\n\n\0", &cmd));
    assert(0 == strcmp("SEND", cmd.name));
    assert(0 == strcmp("topic", cmd.headers->key));
    assert(0 == strcmp("stocks", cmd.headers->value));
    assert(0 == strcmp("old price: 23.4\nnew price: 33.4", cmd.content);

    // missing topic header
    assert(STOMP_MISSING_HEADER
           == parse_command("SEND\n\nnew price: 33.4\n\0", &cmd));

    // no value for topic header
    assert(STOMP_INVALID_HEADER
           == parse_command("SEND\ntopic:\n\n new price: 33.4\n\n\0", &cmd));
    // empty value for topic header
    assert(STOMP_INVALID_HEADER
           == parse_command("SEND\ntopic: \n\n new price: 33.4\n\n\0", &cmd));
}

void test_command_subscribe() {
    struct stomp_command cmd;

    // regular command
    assert(0 == parse_command("SUBSCRIBE\ndestination: stocks\n\n\0", &cmd));
    assert(0 == strcmp("SUBSCRIBE", cmd.name));
    assert(0 == strcmp("destination", cmd.headers->key));
    assert(0 == strcmp("stocks", cmd.headers->value));
    assert(NULL == cmd.content);
 
    // missing topic header
    assert(STOMP_MISSING_HEADER == parse_command("SUBSCRIBE\n\n\0", &cmd));

    // no value for topic header
    assert(STOMP_INVALID_HEADER
           == parse_command("SUBSCRIBE\ndestination:\n\n\0", &cmd));
    // empty value for topic header
    assert(STOMP_INVALID_HEADER
           == parse_command("SUBSCRIBE\ndestination: \n\n\0", &cmd));
 
    // command expects no body
    assert(STOMP_UNEXPECTED_BODY
           == parse_command("SUBSCRIBE\ndestination: stocks\n\nhello\n\n\0", &cmd));
}

// function that takes an error value and
// returns a frame ready to sent to a client

int main(int argc, char** argv) {

    exit(EXIT_SUCCESS);
}
