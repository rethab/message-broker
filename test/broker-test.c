#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <pthread.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/broker.h"
#include "../src/topic.h"
#include "../src/distributor.h"
#include "../src/stomp.h"

void test_send_error() {

    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];

    ret = send_error(&client, "test reason");
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL(
        "ERROR\nmessage:test reason\n\n", rawcmd);

    assert(0 == close(fds[0]));
    assert(0 == close(fds[1]));
    client_destroy(&client);
}

void test_send_receipt() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];

    ret = send_receipt(&client);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", rawcmd);

    assert(0 == close(fds[0]));
    assert(0 == close(fds[1]));
    client_destroy(&client);
}

void test_send_connected() {
    int ret;
    int fds[2]; // 0=read, 1=write
    struct client client;
    char rawcmd[32];

    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];

    ret = send_connected(&client);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    assert(read(fds[0], rawcmd, 32) > 0);
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECTED\n\n", rawcmd);

    assert(0 == close(fds[0]));
    assert(0 == close(fds[1]));
    client_destroy(&client);
}

void test_process_send() {
    int ret;
    struct broker_context ctx ;
    struct stomp_command cmd;
    struct stomp_header header;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct message *msg;
    struct topic *topic;
    struct client client;

    cmd.name = "SEND";
    header.key = "topic";
    header.val = "stocks";
    cmd.headers = &header;
    cmd.nheaders = 1;
    cmd.content = "price: 22.3";
    list_init(&topics);
    ctx.topics = &topics;
    list_init(&messages);
    ctx.messages = &messages;
    client_init(&client);

    assert(0 == topic_add_subscriber(&topics, "stocks", &sub));
    ret = process_send(&ctx, &client, cmd);

    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = messages.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("price: 22.3", msg->content);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);

    topic = topics.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", topic->name);
    client_destroy(&client);
}

void test_process_send_no_subscriber() {
    int ret;
    struct broker_context ctx ;
    struct stomp_command cmd;
    struct stomp_header header;
    struct list topics;
    struct list messages;
    struct client client;
    int fds[2];
    char resp[64];

    cmd.name = "SEND";
    header.key = "topic";
    header.val = "stocks";
    cmd.headers = &header;
    cmd.nheaders = 1;
    cmd.content = "price: 22.3";
    list_init(&topics);
    ctx.topics = &topics;
    list_init(&messages);
    ctx.messages = &messages;
    assert(pipe(fds) == 0);
    client_init(&client);
    client.sockfd = fds[1];

    ret = process_send(&ctx, &client, cmd);

    CU_ASSERT_EQUAL_FATAL(-1, ret);
    assert(0 < read(fds[0], resp, 64));
    CU_ASSERT_STRING_EQUAL_FATAL("ERROR\nmessage:Failed to add message\n\n", resp);

    assert(close(fds[0]) == 0);
    assert(close(fds[1]) == 0);
    client_destroy(&client);
}

void test_process_subscribe() {
    int ret;
    struct broker_context ctx ;
    struct stomp_command cmd;
    struct stomp_header header;
    struct list topics;
    struct list messages;
    struct list *subscribers;
    struct subscriber sub1;
    struct subscriber *sub2;
    struct topic *topic;

    cmd.name = "SUBSCRIBE";
    header.key = "destination";
    header.val = "stocks";
    cmd.headers = &header;
    cmd.nheaders = 1;
    list_init(&topics);
    ctx.topics = &topics;
    list_init(&messages);
    ctx.messages = &messages;
    sub1.name = "x2y";

    ret = process_subscribe(&ctx, cmd, &sub1);

    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_PTR_NULL(messages.root);

    topic = topics.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", topic->name);
    subscribers = topic->subscribers;
    sub2 = subscribers->root->entry;
    CU_ASSERT_EQUAL_FATAL(sub2, &sub1);
}

void test_process_disconnect() {
    int ret;
    struct broker_context ctx ;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct topic *topic;
    struct client client;

    sub.name = "X2Y";

    list_init(&topics);
    ctx.topics = &topics;
    list_init(&messages);
    ctx.messages = &messages;
    client_init(&client);

    topic_add_subscriber(&topics, "stocks", &sub);
    topic_add_message(&topics, &messages, "stocks", "price: 22.3");

    ret = process_disconnect(&ctx, &client, &sub);

    CU_ASSERT_EQUAL_FATAL(0, ret);

    CU_ASSERT(client.dead);

    // topic is not deleted
    topic = topics.root->entry;
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", topic->name);
    client_destroy(&client);
}

void test_process_disconnect_not_subscribed() {
    int ret;
    struct broker_context ctx ;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct client client;

    sub.name = "X2Y";

    list_init(&topics);
    ctx.topics = &topics;
    list_init(&messages);
    client_init(&client);
    ctx.messages = &messages;

    ret = process_disconnect(&ctx, &client, &sub);

    CU_ASSERT_EQUAL_FATAL(0, ret);

    client_destroy(&client);
}

void test_handle_client() {
    struct broker_context ctx;
    struct list topics;
    struct list messages;
    int fds[2];
    struct handler_params hparams;

    assert(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    list_init(&messages);
    list_init(&topics);
    ctx.topics = &topics;
    ctx.messages = &messages;
    hparams.sock = fds[0];
    hparams.ctx = &ctx;

    char cmd1[] = "CONNECT\nlogin:foo\n\n";
    char cmd2[] = "SUBSCRIBE\ndestination:stocks\n\n";
    char cmd3[] = "SEND\ntopic:stocks\n\nprice: 22.3\n\n";
    char cmd4[] = "DISCONNECT\n\n";

    assert(0 < write(fds[1], cmd1, strlen(cmd1)+1));
    assert(0 < write(fds[1], cmd2, strlen(cmd2)+1));
    assert(0 < write(fds[1], cmd3, strlen(cmd3)+1));
    assert(0 < write(fds[1], cmd4, strlen(cmd4)+1));
    handle_client(&hparams);

    size_t resp1len = strlen("CONNECTED\n\n") + 1; // after CONNECT
    char resp1[32]; 
    size_t resp2len = strlen("RECEIPT\n\n") + 1; // after DISCONNECT
    char resp2[32];
    int ret = read(fds[1], resp1, resp1len);
    if (ret != 0) printf("Error: %s\n", strerror(errno));
    assert(0 < ret);
    assert(0 < read(fds[1], resp2, resp2len));
    CU_ASSERT_STRING_EQUAL_FATAL("CONNECTED\n\n", resp1);
    CU_ASSERT_STRING_EQUAL_FATAL("RECEIPT\n\n", resp2);
}

void test_handle_client_first_command_not_connect() {
    struct broker_context ctx;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct client client;
    int connected = 0;
    int fds[2];

    client_init(&client);
    assert(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    client.sockfd = fds[0];
    list_init(&messages);
    list_init(&topics);
    ctx.topics = &topics;
    ctx.messages = &messages;

    char cmd1[] = "SUBSCRIBE\ndestination:stocks\n\n";

    assert(0 < write(fds[1], cmd1, strlen(cmd1)+1));
    CU_ASSERT_EQUAL_FATAL(WORKER_CONTINUE,
        main_loop(&ctx, &client, &connected, &sub));

    size_t resp1len = strlen("ERROR\nmessage:Expected CONNECT\n\n") + 1;
    char resp1[64]; 
    assert(0 < read(fds[1], resp1, resp1len));
    CU_ASSERT_STRING_EQUAL_FATAL("ERROR\nmessage:Expected CONNECT\n\n", resp1);
    list_destroy(&messages);
    list_destroy(&topics);
    client_destroy(&client);
}

void test_handle_client_send_command_unknown() {
    struct broker_context ctx;
    struct list topics;
    struct list messages;
    struct subscriber sub;
    struct client client;
    int connected = 0;
    int fds[2];

    client_init(&client);
    assert(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    client.sockfd = fds[0];
    list_init(&messages);
    list_init(&topics);
    ctx.topics = &topics;
    ctx.messages = &messages;

    char cmd1[] = "UNKNOWN\nfoo\n\n";

    assert(0 < write(fds[1], cmd1, strlen(cmd1)+1));
    CU_ASSERT_EQUAL_FATAL(WORKER_CONTINUE,
        main_loop(&ctx, &client, &connected, &sub));

    size_t resp1len = strlen("ERROR\nmessage:Expected CONNECT\n\n") + 1;
    char resp1[64]; 
    assert(0 < read(fds[1], resp1, resp1len));
    CU_ASSERT_STRING_EQUAL_FATAL("ERROR\nmessage:Expected CONNECT\n\n", resp1);
    list_destroy(&messages);
    list_destroy(&topics);
    client_destroy(&client);
}

void test_handle_client_dead() {
    struct broker_context ctx ;
    struct list topics;
    struct list messages;
    int fds[2];
    struct handler_params hparams;

    ctx.topics = &topics;
    ctx.messages = &messages;
    hparams.sock = fds[0];
    hparams.ctx = &ctx;

    handle_client(&hparams);

    // no assertions, but would block
    // if it was not working
}

void test_handle_client_too_much() {
    struct broker_context ctx;
    struct list topics;
    struct list messages;
    int fds[2];
    struct handler_params hparams;

    assert(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
    list_init(&messages);
    list_init(&topics);
    ctx.topics = &topics;
    ctx.messages = &messages;
    hparams.sock = fds[0];
    hparams.ctx = &ctx;

    char *cmd = malloc(4096);
    memset(cmd, 1, 4095);
    cmd[4095] = '\0';

    assert(0 < write(fds[1], cmd, 4096));
    handle_client(&hparams);

    // should have been closed
    CU_ASSERT_EQUAL_FATAL(-1, close(fds[0]));
}

void test_init_destory_context() {
    int ret;

    struct broker_context ctx;
    struct message msg;
    struct topic topic;

    ret = broker_context_init(&ctx);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    ret = list_add(ctx.messages, &msg);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    ret = list_add(ctx.topics, &topic);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    
    assert(0 == list_remove(ctx.messages, &msg));
    assert(0 == list_remove(ctx.topics, &topic));
    ret = broker_context_destroy(&ctx);
    CU_ASSERT_EQUAL_FATAL(0, ret);
}

void test_deliver_after_disconnect() {
    int ret;
    struct stomp_command cmd;
    struct stomp_header header;
    struct broker_context ctx ;
    struct subscriber sub;
    struct client client;

    cmd.name = strdup("SEND");
    header.key = strdup("topic");
    header.val = strdup("stocks");
    cmd.headers = &header;
    cmd.nheaders = 1;
    cmd.content = strdup("price: 22.3");
    broker_context_init(&ctx);
    client_init(&client);
    sub.name = strdup("foo");
    sub.client = &client;

    ret = process_subscribe(&ctx, cmd, &sub);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    ret = process_send(&ctx, &client, cmd);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    ret = process_disconnect(&ctx, &client, &sub);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    deliver_messages(ctx.messages);

    CU_ASSERT_FATAL(client.dead);
    CU_ASSERT_PTR_NOT_NULL_FATAL(client.mutex_w);
    CU_ASSERT_PTR_NOT_NULL_FATAL(client.mutex_r);
    CU_ASSERT_PTR_NOT_NULL_FATAL(client.deadmutex);

    client_destroy(&client);
}

void broker_test_suite() {
    CU_pSuite socketSuite = CU_add_suite("broker", NULL, NULL);
    CU_add_test(socketSuite, "test_process_send", test_process_send);
    CU_add_test(socketSuite, "test_process_send_no_subscriber", test_process_send_no_subscriber);
    CU_add_test(socketSuite, "test_process_subscribe",
        test_process_subscribe);
    CU_add_test(socketSuite, "test_process_disconnect",
        test_process_disconnect);
    CU_add_test(socketSuite, "test_process_disconnect_not_subscribed",
        test_process_disconnect_not_subscribed);
    CU_add_test(socketSuite, "test_send_connected", test_send_connected);
    CU_add_test(socketSuite, "test_send_receipt", test_send_receipt);
    CU_add_test(socketSuite, "test_send_error", test_send_error);
    CU_add_test(socketSuite, "test_handle_client", test_handle_client);
    CU_add_test(socketSuite, "test_handle_client_dead",
        test_handle_client_dead);
    CU_add_test(socketSuite, "test_handle_client_too_much",
        test_handle_client_too_much);
    CU_add_test(socketSuite,
        "test_handle_client_first_command_not_connect",
        test_handle_client_first_command_not_connect);
    CU_add_test(socketSuite,
        "test_handle_client_send_command_unknown",
        test_handle_client_send_command_unknown);

    CU_add_test(socketSuite, "test_init_destory_context",
        test_init_destory_context);
    CU_add_test(socketSuite, "test_deliver_after_disconnect",
        test_deliver_after_disconnect);
}
