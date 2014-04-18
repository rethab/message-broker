#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/distributor.h"

#define HOUR (60*60)
#define DAY (60*60*24)

static struct list messages;
static struct message msg1;
static struct message msg2;
static struct msg_statistics stat1;
static struct msg_statistics stat2;
static struct msg_statistics stat3;
static struct subscriber sub1;
static struct subscriber sub2;
static struct client client1;
static struct client client2;
static int fds1[2];
static int fds2[2];

static int before_test() {
    list_init(&messages);

    message_init(&msg1);
    message_init(&msg2);
    msg1.topicname = "stocks";
    msg2.topicname = "stocks";
    msg1.content = "price:23.3";
    msg2.content = "price:22.2";
    list_add(&messages, &msg1);
    list_add(&messages, &msg2); 

    msg_statistics_init(&stat1);
    msg_statistics_init(&stat2);
    msg_statistics_init(&stat3);
    list_add(msg1.stats, &stat1);
    list_add(msg2.stats, &stat2);
    list_add(msg2.stats, &stat3);

    client_init(&client1);
    client_init(&client2);
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds1);
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds2);
    client1.sockfd = fds1[0];
    client2.sockfd = fds2[0];
    sub1.client = &client1;
    sub2.client = &client2;
    stat1.subscriber = &sub1;
    stat2.subscriber = &sub1;
    stat3.subscriber = &sub2;
    return 0;
}

static int after_test() {
    client_destroy(&client1);
    client_destroy(&client2);
    close(fds1[0]);
    close(fds1[1]);
    close(fds2[0]);
    close(fds2[1]);
    list_remove(msg1.stats, &stat1);
    list_remove(msg2.stats, &stat2);
    list_remove(msg2.stats, &stat3);
    msg_statistics_destroy(&stat1);
    msg_statistics_destroy(&stat2);
    msg_statistics_destroy(&stat3);

    list_remove(&messages, &msg1);
    list_remove(&messages, &msg2);
    message_destroy(&msg1);
    message_destroy(&msg2);

    list_destroy(&messages);
    return 0;
}

static long now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void test_deliver_messages() {
    before_test();
    // never tried before
    stat1.nattempts = 0;
    stat1.last_fail = 0;

    // long ago
    stat2.nattempts = 1;
    stat2.last_fail = now() - HOUR;

    // very long ago
    stat3.nattempts = 3;
    stat3.last_fail = now() - DAY;

    deliver_messages(messages);
    CU_ASSERT_EQUAL_FATAL(1, stat1.nattempts);
    CU_ASSERT_EQUAL_FATAL(2, stat2.nattempts);
    CU_ASSERT_EQUAL_FATAL(4, stat3.nattempts);
    CU_ASSERT_EQUAL_FATAL(0, stat1.last_fail);
    CU_ASSERT_EQUAL_FATAL(0, stat2.last_fail);
    CU_ASSERT_EQUAL_FATAL(0, stat3.last_fail);

    size_t nbytes;
    char msgbuf[64];
    nbytes = read(fds1[1], msgbuf, 41);
    assert(nbytes > 0);
    CU_ASSERT_STRING_EQUAL_FATAL(
        "MESSAGE\ndestination:stocks\n\nprice:23.3\n\n", msgbuf);
    assert(0 < read(fds1[1], msgbuf, 41));
    CU_ASSERT_STRING_EQUAL_FATAL(
        "MESSAGE\ndestination:stocks\n\nprice:22.2\n\n", msgbuf);
    assert(0 < read(fds2[1], msgbuf, 41));
    CU_ASSERT_STRING_EQUAL_FATAL(
        "MESSAGE\ndestination:stocks\n\nprice:22.2\n\n", msgbuf);
    after_test();
}

void test_deliver_messages_not_eligible() {
    before_test();
    // too many attempts: dont deliver
    stat1.nattempts = 999;
    long stat1_fail = now() - HOUR;
    stat1.last_fail = stat1_fail;

    // just tried: dont deliver
    stat2.nattempts = 1;
    long stat2_fail = now();
    stat2.last_fail = stat2_fail;

    // very long ago: deliver
    stat3.nattempts = 3;
    stat3.last_fail = now() - DAY;

    deliver_messages(messages);
    CU_ASSERT_EQUAL_FATAL(999, stat1.nattempts);
    CU_ASSERT_EQUAL_FATAL(1, stat2.nattempts);
    CU_ASSERT_EQUAL_FATAL(4, stat3.nattempts);
    CU_ASSERT_EQUAL_FATAL(stat1_fail, stat1.last_fail); // not changed
    CU_ASSERT_EQUAL_FATAL(stat2_fail, stat2.last_fail); // not changed
    CU_ASSERT_EQUAL_FATAL(0, stat3.last_fail);

    char msgbuf[64];
    assert(0 < read(fds2[1], msgbuf, 46));
    CU_ASSERT_STRING_EQUAL_FATAL(
        "MESSAGE\ndestination:stocks\n\nprice:22.2\n\n", msgbuf);
    after_test();
}

void test_deliver_message_already_delivered() {
    before_test();
    // already successfully sent
    stat1.nattempts = 1;

    // just tried: dont deliver
    stat2.nattempts = 1;
    long stat2_fail = now();
    stat2.last_fail = stat2_fail;

    // very long ago: deliver
    stat3.nattempts = 3;
    stat3.last_fail = now() - DAY;

    deliver_messages(messages);
    CU_ASSERT_EQUAL_FATAL(1, stat1.nattempts);
    CU_ASSERT_EQUAL_FATAL(1, stat2.nattempts);
    CU_ASSERT_EQUAL_FATAL(4, stat3.nattempts);
    CU_ASSERT_EQUAL_FATAL(0, stat1.last_fail); // not changed
    CU_ASSERT_EQUAL_FATAL(stat2_fail, stat2.last_fail); // not changed
    CU_ASSERT_EQUAL_FATAL(0, stat3.last_fail);

    char msgbuf[64];
    assert(0 < read(fds2[1], msgbuf, 46));
    CU_ASSERT_STRING_EQUAL_FATAL(
        "MESSAGE\ndestination:stocks\n\nprice:22.2\n\n", msgbuf);
    after_test();
}

void test_handle_closed_socket_and_dead_client() {
    before_test();
    // never tried before
    stat1.nattempts = 0;
    stat1.last_fail = 0;

    // long ago
    stat2.nattempts = 1;
    stat2.last_fail = now() - HOUR;

    // very long ago
    stat3.nattempts = 3;
    stat3.last_fail = now() - DAY;

    // client1 is dead, client2 has closed socket
    client1.dead = 1;
    assert(close(fds2[0]) == 0);

    deliver_messages(messages);

    CU_ASSERT(client2.dead);
    after_test();
}

void distributor_test_suite() {
    CU_pSuite distrSuite =
        CU_add_suite("distributor", NULL, NULL);
    CU_add_test(distrSuite, "test_deliver_messages",
        test_deliver_messages);
    CU_add_test(distrSuite, "test_deliver_messages_not_eligible",
        test_deliver_messages_not_eligible);
    CU_add_test(distrSuite, "test_handle_closed_socket_and_dead_client",
        test_handle_closed_socket_and_dead_client);
    CU_add_test(distrSuite, "test_deliver_message_already_delivered",
        test_deliver_message_already_delivered);

}
