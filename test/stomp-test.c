#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>

#include "../src/stomp.h"

/*   TEST PARSING OF CLIENT REQUESTS */
void test_parse_command_connect() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0,
        parse_command("CONNECT\nlogin: client-1\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("CONNECT", cmd.name);
    CU_ASSERT_STRING_EQUAL("login", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("client-0", cmd.headers->value);
    CU_ASSERT_PTR_NULL(cmd.content);

    // missing login header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER,
        parse_command("CONNECT\n\n\0", &cmd));

    // no value for login header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("CONNECT\nlogin: \0", &cmd));
 
    // command expects no body
    CU_ASSERT_EQUAL(STOMP_UNEXPECTED_BODY,
        parse_command("CONNECT\nlogin: client-1\n\nhello\n\0", &cmd));
}

void test_parse_command_send() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0,
        parse_command("SEND\ntopic: stocks\n\nnew price: 33.4\n\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_STRING_EQUAL("new price: 33.4", cmd.content);
 
    // multiline regular command
    CU_ASSERT_EQUAL(0,
        parse_command("SEND\ntopic: stocks\n\no p: 23.4\nn p: 33.4\n\n\0",
                      &cmd));
    CU_ASSERT_STRING_EQUAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_STRING_EQUAL("o p: 23.4\nn p: 33.4", cmd.content);

    // missing topic header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER,
        parse_command("SEND\n\nnew price: 33.4\n\0", &cmd));

    // no value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("SEND\ntopic:\n\n new price: 33.4\n\n\0", &cmd));
    // empty value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("SEND\ntopic: \n\n new price: 33.4\n\n\0", &cmd));
}

void test_parse_command_subscribe() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0,
        parse_command("SUBSCRIBE\ndestination: stocks\n\n\0", &cmd));
    CU_ASSERT_STRING_EQUAL("SUBSCRIBE", cmd.name);
    CU_ASSERT_STRING_EQUAL("destination", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL("stocks", cmd.headers->value);
    CU_ASSERT_PTR_NULL(cmd.content);
 
    // missing topic header
    CU_ASSERT_EQUAL(STOMP_MISSING_HEADER,
        parse_command("SUBSCRIBE\n\n\0", &cmd));

    // no value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("SUBSCRIBE\ndestination:\n\n\0", &cmd));
    // empty value for topic header
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("SUBSCRIBE\ndestination: \n\n\0", &cmd));
 
    // command expects no body
    CU_ASSERT_EQUAL(STOMP_UNEXPECTED_BODY,
        parse_command("SUBSCRIBE\ndestination: stocks\n\nhello\n\n\0", &cmd));
}

void test_parse_command_disconnect() {
    struct stomp_command cmd;

    // regular command
    CU_ASSERT_EQUAL(0, parse_command("DISCONNECT", &cmd));

    // invalid header (wants none)
    CU_ASSERT_EQUAL(STOMP_INVALID_HEADER,
        parse_command("DISCONNECT\nfoo: bar\n\n\0", &cmd));

    CU_ASSERT_EQUAL(STOMP_UNEXPECTED_BODY,
        parse_command("DISCONNECT\n\nbody\n\n\0", &cmd));
}


/* TEST CREATIONS FOR CLIENT RESPONSE */

void test_create_command_connected() {
    struct stomp_command cmd;   

    cmd.name = "CONNECTED";
    cmd.headers = NULL;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL("CONNECTED", str);
}

void test_create_command_error() {
    struct stomp_command cmd;   

    struct stomp_header msg;
    msg.key = "message";
    msg.value = "fail";
    cmd.name = "ERROR";
    cmd.headers = &msg;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL("ERROR\nmesage:failed\n\n\0", str);
}

void test_create_command_message() {
    struct stomp_command cmd;   

    // one line
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello world";

    char* str;
    CU_ASSERT_EQUAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL("MESSAGE\n\nhello world\n\n\0", str);

    // two lines
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello\n world";

    CU_ASSERT_EQUAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL("MESSAGE\n\nhello\n world\n\n\0", str);
}

void test_create_command_receipt() {
    struct stomp_command cmd;

    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL("RECEIPT\n\n\0", str);
}


// function that takes an error value and
// returns a frame ready to sent to a client

int main(int argc, char** argv) {
    CU_pSuite parseSuite = CU_add_suite("stomp_parse", NULL, NULL);
    CU_add_test(parseSuite, "test_parse_command_connect",
        test_parse_command_connect);
    CU_add_test(parseSuite, "test_parse_command_send",
        test_parse_command_send);
    CU_add_test(parseSuite, "test_parse_command_subscribe",
        test_parse_command_subscribe);
    CU_add_test(parseSuite, "test_parse_command_disconnect",
        test_parse_command_disconnect);

    CU_pSuite createSuite = CU_add_suite("stomp_create", NULL, NULL);
    CU_add_test(createSuite, "test_create_command_connected",
        test_create_command_connected);
    CU_add_test(createSuite, "test_create_command_error",
        test_create_command_error);
    CU_add_test(createSuite, "test_create_command_message",
        test_create_command_message);
    CU_add_test(createSuite, "test_create_command_receipt",
        test_create_command_receipt);

    CU_basic_run_suite(parseSuite);
    CU_basic_run_suite(createSuite);
    exit(EXIT_SUCCESS);
}
