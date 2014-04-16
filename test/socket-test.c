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
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[0];
    client.mutex_r = &mutex;

    assert(write(fds[1], rawcmd, rawcmdlen) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECT", cmd.name);
    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_read_command_fail() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[] = "CONNECT\n\n"; // missing header
    size_t rawcmdlen = strlen(rawcmd) + 1;

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[0];
    client.mutex_r = &mutex;

    assert(write(fds[1], rawcmd, rawcmdlen) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT(ret < 0); // failed to parse
    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_read_command_invalid_socket() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[0];
    client.mutex_r = &mutex;

    // close both
    close(fds[0]);
    close(fds[1]);

    ret = socket_read_command(&client, &cmd);

    CU_ASSERT(ret < 0); // failed to parse
    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    pthread_mutex_destroy(&mutex);
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
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[0];
    client.mutex_r = &mutex;

    assert(write(fds[1], rawcmd, 1025) > 0);
    ret = socket_read_command(&client, &cmd);

    CU_ASSERT_EQUAL_FATAL(SOCKET_TOO_MUCH, ret);
    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_send_command() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[1];
    client.mutex_w = &mutex;
    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.nheaders = 0;
    cmd.content = NULL;

    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    read(fds[0], &rawcmd, 32);
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", rawcmd);

    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_send_command_socket_closed() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    struct stomp_command cmd;

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[1];
    client.mutex_w = &mutex;
    cmd.name = "RECEIPT";
    cmd.headers = NULL;
    cmd.nheaders = 0;
    cmd.content = NULL;

    close(fds[1]); // close socket
    close(fds[0]);

    ret = socket_send_command(&client, cmd);
    CU_ASSERT_EQUAL_FATAL(SOCKET_CLIENT_GONE, ret);
    // mutex should be freed again
    CU_ASSERT_EQUAL_FATAL(0, pthread_mutex_trylock(&mutex));

    pthread_mutex_destroy(&mutex);
}


void socket_test_suite() {
    CU_pSuite socketSuite = CU_add_suite("socket", NULL, NULL);
    CU_add_test(socketSuite, "test_read_command", test_read_command);
    CU_add_test(socketSuite, "test_read_command_fail", test_read_command_fail);
    CU_add_test(socketSuite, "test_read_command_invalid_socket",
        test_read_command_invalid_socket);
    CU_add_test(socketSuite, "test_read_command_too_much",
        test_read_command_too_much);
    CU_add_test(socketSuite, "test_send_command_socket_closed",
        test_send_command_socket_closed);
    CU_add_test(socketSuite, "test_send_command", test_send_command);

}
