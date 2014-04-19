#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/stomp.h"

/*   TEST PARSING OF CLIENT REQUESTS */
void test_parse_command_unknown() {
    struct stomp_command cmd;

    // unknown command
    char str[] = "FOO\nlogin: client-1\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNKNOWN_COMMAND,
        parse_command(str, &cmd));
}

void test_parse_command_connect() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "CONNECT\nlogin: client-1\n\n";
    int parsed = parse_command(str1, &cmd);
    CU_ASSERT_EQUAL_FATAL(0, parsed);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECT", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("login", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("client-1", cmd.headers->val);
    CU_ASSERT_EQUAL_FATAL(1, cmd.nheaders);
    CU_ASSERT_PTR_NULL_FATAL(cmd.content);
    
    // regular command with whitespaces in header
    char str2[] = "CONNECT\n login :  client-1  \n\n";
    parsed = parse_command(str2, &cmd);
    CU_ASSERT_EQUAL_FATAL(0, parsed);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECT", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("login", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("client-1", cmd.headers->val);
    CU_ASSERT_EQUAL_FATAL(1, cmd.nheaders);
    CU_ASSERT_PTR_NULL_FATAL(cmd.content);

    // missing login header
    char str3[] = "CONNECT\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str3, &cmd));

    // no val for login header
    char str4[] = "CONNECT\nlogin: \n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str4, &cmd));
 
    // command expects no body
    char str5[] = "CONNECT\nlogin: client-1\n\nhello\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNEXPECTED_CONTENT,
        parse_command(str5, &cmd));
    // additional header
    char str6[] = "CONNECT\nlogin: client-1\nfoo: bar\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str6, &cmd));
 
    // wrong header
    char str7[] = "CONNECT\nfoo: bar\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str7, &cmd));
}

void test_parse_command_send() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "SEND\ntopic: stocks\n\nnew price: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str1, &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_EQUAL_FATAL(1, cmd.nheaders);
    CU_ASSERT_STRING_EQUAL_FATAL("new price: 33.4", cmd.content);
 
    // multiline regular command
    char str2[] = "SEND\ntopic: stocks\n\no p: 23.4\nn p: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str2,
                      &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SEND", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("topic", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_EQUAL_FATAL(1, cmd.nheaders);
    CU_ASSERT_STRING_EQUAL_FATAL("o p: 23.4\nn p: 33.4", cmd.content);

    // missing topic header
    char str3[] = "SEND\n\nnew price: 33.4\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str3, &cmd));

    // no value for topic header
    char str4[] = "SEND\ntopic:\n\n new price: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str4, &cmd));

    // empty value for topic header
    char str5[] = "SEND\ntopic: \n\n new price: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str5, &cmd));

    // wrong header
    char str6[] = "SEND\ntropic: foo\n\n new price: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str6, &cmd));
    
    // additional header
    char str7[] = "SEND\ntopic: foo\nbar:wrapm\n\n new price: 33.4\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str7, &cmd));
}

void test_parse_command_subscribe() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "SUBSCRIBE\ndestination: stocks\n\n";
    CU_ASSERT_EQUAL_FATAL(0,
        parse_command(str1, &cmd));
    CU_ASSERT_STRING_EQUAL_FATAL("SUBSCRIBE", cmd.name);
    CU_ASSERT_STRING_EQUAL_FATAL("destination", cmd.headers->key);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", cmd.headers->val);
    CU_ASSERT_EQUAL_FATAL(1, cmd.nheaders);
    CU_ASSERT_PTR_NULL_FATAL(cmd.content);
 
    // missing topic header
    char str2[] = "SUBSCRIBE\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_MISSING_HEADER,
        parse_command(str2, &cmd));

    // no value for topic header
    char str3[] = "SUBSCRIBE\ndestination:\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str3, &cmd));
    // empty value for topic header
    char str4[] = "SUBSCRIBE\ndestination: \n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str4, &cmd));
 
    // command expects no body
    char str5[] = "SUBSCRIBE\ndestination: stocks\n\nhello\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_UNEXPECTED_CONTENT,
        parse_command(str5, &cmd));
    
    // wrong header
    char str6[] = "SUBSCRIBE\ntopic: stocks\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str6, &cmd));
    
    // additional header
    char str7[] = "SUBSCRIBE\ntopic: stocks\ndestination: stocks\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str7, &cmd));
}

void test_parse_command_disconnect() {
    struct stomp_command cmd;

    // regular command
    char str1[] = "DISCONNECT";
    CU_ASSERT_EQUAL_FATAL(0, parse_command(str1, &cmd));
    CU_ASSERT_EQUAL_FATAL(0, cmd.nheaders);

    // invalid header (wants none)
    char str2[] = "DISCONNECT\nfoo: bar\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str2, &cmd));

    char str3[] = "DISCONNECT\nfoo:bar\n\nbody\n\n";
    CU_ASSERT_EQUAL_FATAL(STOMP_INVALID_HEADER,
        parse_command(str3, &cmd));
}


/* TEST CREATIONS FOR CLIENT RESPONSE */

void test_create_command_connected() {
    struct stomp_command cmd;   

    cmd.name = "CONNECTED";
    cmd.headers = NULL;
    cmd.content = NULL;
    cmd.nheaders = 0;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECTED\n\n", str);
}

void test_create_command_error() {
    struct stomp_command cmd;   

    struct stomp_header hdr;
    hdr.key = "message";
    hdr.val = "fail";
    cmd.name = "ERROR";
    cmd.headers = &hdr;
    cmd.content = NULL;
    cmd.nheaders = 1;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("ERROR\nmessage:fail\n\n", str);
}

void test_create_command_message() {
    struct stomp_command cmd;   

    // one line
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello world";
    cmd.nheaders = 0;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("MESSAGE\nhello world\n\n", str);

    // two lines
    cmd.name = "MESSAGE";
    cmd.headers = NULL;
    cmd.content = "hello\n world";
    cmd.nheaders = 0;

    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("MESSAGE\nhello\n world\n\n", str);
}

void test_create_command_receipt() {
    struct stomp_command cmd;

    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.content = NULL;
    cmd.nheaders = 0;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", str);
}

void test_create_command_strcatbug() {
    /** this demonstrates a bug in create_command_generic
     * that was fixed in 3346752. The problem
     * was that an assumption on the behavior of strcat
     * was wrong, which caused it to only work
     * if the memory returned by malloc had a zero
     * byte at the beginning. Therefore, we first
     * allocate some memory and set it to 1 and then
     * free it, which makes it very likely to be returned
     * later on and not be zero. only memory from the OS
     * will be zeroed out, if possible memory is reused */
    int *dummy = malloc(1024 * sizeof(int));
    assert(dummy != NULL);
    for (int i = 0; i < 1024; i++)
        dummy[i] = 1;
    free(dummy);

    struct stomp_command cmd;

    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.content = NULL;
    cmd.nheaders = 0;

    char* str;
    CU_ASSERT_EQUAL_FATAL(0, create_command(cmd, &str));
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", str);
}

void test_strerror() {
    char buf[32];
    stomp_strerror(STOMP_MISSING_HEADER, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_MISSING_HEADER", buf);

    stomp_strerror(STOMP_INVALID_HEADER, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_INVALID_HEADER", buf);

    stomp_strerror(STOMP_UNEXPECTED_HEADER, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_UNEXPECTED_HEADER", buf);

    stomp_strerror(STOMP_UNEXPECTED_CONTENT, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_UNEXPECTED_CONTENT", buf);

    stomp_strerror(STOMP_MISSING_CONTENT, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_MISSING_CONTENT", buf);

    stomp_strerror(STOMP_INVALID_CONTENT, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_INVALID_CONTENT", buf);

    stomp_strerror(STOMP_UNKNOWN_COMMAND, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("STOMP_UNKNOWN_COMMAND", buf);

    stomp_strerror(-1, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("UNKNOWN_ERROR", buf);
}

void add_stomp_parse_suite() {
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
}

void add_stomp_create_suite() {
    CU_pSuite createSuite = CU_add_suite("stomp_create", NULL, NULL);
    CU_add_test(createSuite, "test_create_command_connected",
        test_create_command_connected);
    CU_add_test(createSuite, "test_create_command_error",
        test_create_command_error);
    CU_add_test(createSuite, "test_create_command_message",
        test_create_command_message);
    CU_add_test(createSuite, "test_create_command_receipt",
        test_create_command_receipt);
    CU_add_test(createSuite, "test_create_command_strcatbug",
        test_create_command_strcatbug);
    CU_add_test(createSuite, "test_strerror",
        test_strerror);
}
