#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/worker.h"
#include "../src/topic.h"

void test_send_error() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[1];
    client.mutex_w = &mutex;

    ret = send_error(&client, "test reason");
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL(
        "ERROR\nmessage:test reason\n\n", rawcmd);

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_send_receipt() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[1];
    client.mutex_w = &mutex;

    ret = send_receipt(&client);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL( "RECEIPT\n\n", rawcmd);

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_send_connected() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct worker_params params;
    struct client client;
    params.client = &client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    client.sockfd = fds[1];
    client.mutex_w = &mutex;

    printf("ADdress in test: Lock: %p\n", &mutex);

    ret = send_connected(params);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECTED\n\n", rawcmd);

    close(fds[0]);
    close(fds[1]);
    pthread_mutex_destroy(&mutex);
}

void test_process_send() {
    int ret;
    struct worker_params params;
    struct stomp_command cmd;
    struct stomp_header header;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct message *msg;
    struct topic *topic;

    cmd.name = "SEND";
    header.key = "topic";
    header.val = "stocks";
    cmd.headers = &header;
    cmd.nheaders = 1;
    cmd.content = "price: 22.3";
    list_init(&topics);
    params.topics = &topics;
    list_init(&messages);
    params.messages = &messages;

    assert(0 == topic_add_subscriber(&topics, "stocks", &sub));
    ret = process_send(params, cmd);

    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = messages.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("price: 22.3", msg->content);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);

    topic = topics.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", topic->name);
}


void worker_test_suite() {
    CU_pSuite socketSuite = CU_add_suite("worker", NULL, NULL);
    //CU_add_test(socketSuite, "test_send_error", test_send_error);
    CU_add_test(socketSuite, "test_send_connected", test_send_connected);
    //CU_add_test(socketSuite, "test_send_receipt", test_send_receipt);
    //CU_add_test(socketSuite, "test_process_send", test_process_send);

}
