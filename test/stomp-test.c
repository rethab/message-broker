#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "util.c"
#include "../src/stomp.h"

/*   TEST PARSING OF CLIENT REQUESTS */
void test_parse_command_unknown() {
    struct stomp_command cmd;

    // unknown command
    char str[] = "FOO\nlogin: client-1\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNKNOWN_COMMAND,
        parse_command(str, &cmd));
}

void test_parse_command_connect() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "CONNECT\nlogin: client-1\n\n\0";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str1, &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECT", cmd.name);
    printf("Header: %s\n", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("login", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("client-0", cmd.headers->val);
    CU_ASSERT_PTR_NULL_FATAL(cmd.content);

    // missing login header
    char str2[] = "CONNECT\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str2, &cmd));

    // no val for login header
    char str3[] = "CONNECT\nlogin: \0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str3, &cmd));
 
    // command expects no body
    char str4[] = "CONNECT\nlogin: client-1\n\nhello\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNEXPECTED_BODY,
        parse_command(str4, &cmd));
}

void test_parse_command_send() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "SEND\ntopic: stocks\n\nnew price: 33.4\n\n\0";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str1, &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_STRING_EQUAL_FATAL("new price: 33.4", cmd.content);
 
    // multiline regular command
    char str2[] = "SEND\ntopic: stocks\n\no p: 23.4\nn p: 33.4\n\n\0";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str2,
                      &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_STRING_EQUAL_FATAL("o p: 23.4\nn p: 33.4", cmd.content);

    // missing topic header
    char str3[] = "SEND\n\nnew price: 33.4\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str3, &cmd));

    // no value for topic header
    char str4[] = "SEND\ntopic:\n\n new price: 33.4\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str4, &cmd));
    // empty value for topic header
    char str5[] = "SEND\ntopic: \n\n new price: 33.4\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str5, &cmd));
}

void test_parse_command_subscribe() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "SUBSCRIBE\ndestination: stocks\n\n\0";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str1, &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SUBSCRIBE", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("destination", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_PTR_NULL_FATAL(cmd.content);
 
    // missing topic header
    char str2[] = "SUBSCRIBE\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str2, &cmd));

    // no value for topic header
    char str3[] = "SUBSCRIBE\ndestination:\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str3, &cmd));
    // empty value for topic header
    char str4[] = "SUBSCRIBE\ndestination: \n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str4, &cmd));
 
    // command expects no body
    char str5[] = "SUBSCRIBE\ndestination: stocks\n\nhello\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNEXPECTED_BODY,
        parse_command(str5, &cmd));
}

void test_parse_command_disconnect() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "DISCONNECT";
    CU_ASSERT_EQUAL_FATAL(0, parse_command(str1, &cmd));

    // invalid header (wants none)
    char str2[] = "DISCONNECT\nfoo: bar\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str2, &cmd));

    char str3[] = "DISCONNECT\n\nbody\n\n\0";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNEXPECTED_BODY,
        parse_command(str3, &cmd));
}


/* TEST CREATIONS FOR CLIENT RESPONSE */

void test_create_command_connected() {
    struct stomp_command cmd;   

    cmd.name = "CONNECTED";
    cmd.headers = NULL;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECTED", str);
}

void test_create_command_error() {
    struct stomp_command cmd;   

    struct stomp_header msg;
    msg.key = "message";
    msg.val = "fail";
    cmd.name = "ERROR";
    cmd.headers = &msg;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL_FATAL("ERROR\nmesage:failed\n\n\0", str);
}

void test_create_command_message() {
    struct stomp_command cmd;   

    // one line
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello world";

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL_FATAL("MESSAGE\n\nhello world\n\n\0", str);

    // two lines
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello\n world";

    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL_FATAL("MESSAGE\n\nhello\n world\n\n\0", str);
}

void test_create_command_receipt() {
    struct stomp_command cmd;

    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.content = NULL;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, str));
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n\0", str);
}


// function that takes an error value and
// returns a frame ready to sent to a client

int main(int argc, char** argv) {
    install_segfault_handler();

    assert(CUE_SUCCESS == CU_initialize_registry());

    CU_pSuite parseSuite = CU_add_suite("stomp_parse", NULL, NULL);
    CU_add_test(parseSuite, "test_parse_command_unknown",
        test_parse_command_unknown);
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
    
    CU_basic_run_tests();

    CU_cleanup_registry();
    exit(EXIT_SUCCESS);
}
