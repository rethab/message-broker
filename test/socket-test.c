#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/socket.h"

void test_read_command() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[] = "CONNECT\nlogin:foo\n\n";
    size_t rawcmdlen = strlen(rawcmd) + 1;

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[0];

    assert(write(fds[1], rawcmd, rawcmdlen) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECT", cmd.name);

    close(fds[0]);
    close(fds[1]);
    client_destroy(&client);
}

void test_read_command_fail() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[] = "CONNECT\n\n"; // missing header
    size_t rawcmdlen = strlen(rawcmd) + 1;

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[0];

    assert(write(fds[1], rawcmd, rawcmdlen) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT(ret < 0); // failed to parse

    close(fds[0]);
    close(fds[1]);
    client_destroy(&client);
}

void test_read_command_invalid_socket() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[0];

    // close both
    close(fds[0]);
    close(fds[1]);

    ret = socket_read_command(&client, &cmd);

    CU_ASSERT(ret < 0); // failed to parse
    CU_ASSERT_EQUAL_FATAL(1, client.dead);

    client_destroy(&client);
}

void test_read_or_write_to_dead_client() {
    int ret;
    struct client client;
    struct stomp_command cmd;
    cmd.name = "RECEIPT";
    cmd.nheaders = 0;
    cmd.content = NULL;

    client_init(&client);
    client.dead = 1;

    // cannot engage in necromantic activities
    ret = socket_read_command(&client, &cmd);
    CU_ASSERT_EQUAL_FATAL(SOCKET_NECROMANCE, ret);

    // cannot engage in necromantic activities
    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(SOCKET_NECROMANCE, ret);

    client_destroy(&client);
}

void test_send_invalid_command() {
    int ret;
    struct client client;
    struct stomp_command cmd;
    cmd.name = "FOO";
    cmd.nheaders = 0;
    cmd.content = NULL;

    client_init(&client);

    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(-1, ret);

    client_destroy(&client);
}

void test_read_command_too_much() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[1025];
    for (int i = 0; i < 1024; i++) {
        rawcmd[i] = 'a';
    }
    rawcmd[1024] = '\0';

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[0];

    assert(write(fds[1], rawcmd, 1025) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT_EQUAL_FATAL(SOCKET_TOO_MUCH, ret);

    close(fds[0]);
    close(fds[1]);
    client_destroy(&client);
}

void test_send_command() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];
    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.nheaders = 0;
    cmd.content = NULL;

    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    read(fds[0], &rawcmd, 32);
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", rawcmd);

    close(fds[0]);
    close(fds[1]);
    client_destroy(&client);
}

void test_send_command_socket_closed() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];
    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.nheaders = 0;
    cmd.content = NULL;

    assert(0 == close(fds[1])); // close socket
    assert(0 == close(fds[0]));

    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(SOCKET_CLIENT_GONE, ret);

    client_destroy(&client);
}

void test_terminate_client() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];

    ret = socket_terminate_client(&client);
    assert(ret == 0);

    CU_ASSERT_EQUAL_FATAL(1, client.dead);
    ret = close(fds[1]);
    // should already be closed
    CU_ASSERT_EQUAL_FATAL(-1, ret);
    CU_ASSERT_EQUAL_FATAL(EBADF, errno);

    assert(0 == close(fds[0]));

    client_destroy(&client);
}


void socket_test_suite() {
    CU_pSuite socketSuite = CU_add_suite("socket", NULL, NULL);
    CU_add_test(socketSuite, "test_read_command", test_read_command);
    CU_add_test(socketSuite, "test_read_command_fail", test_read_command_fail);
    CU_add_test(socketSuite, "test_read_or_write_to_dead_client", test_read_or_write_to_dead_client);
    CU_add_test(socketSuite, "test_read_command_invalid_socket",
        test_read_command_invalid_socket);
    CU_add_test(socketSuite, "test_read_command_too_much",
        test_read_command_too_much);
    CU_add_test(socketSuite, "test_send_command_socket_closed",
        test_send_command_socket_closed);
    CU_add_test(socketSuite, "test_send_command", test_send_command);
    CU_add_test(socketSuite, "test_terminate_client",
        test_terminate_client);
    CU_add_test(socketSuite, "test_send_invalid_command",
        test_send_invalid_command);
}
