#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/topic.h"

static struct client c1;
static struct client c2;
static struct client c3;

static void topic_before_test() {
    client_init(&c1);
    client_init(&c2);
    client_init(&c3);

    c1.dead = 0;
    c2.dead = 0;
    c3.dead = 0;
}

static void topic_after_test() {
    client_destroy(&c1);
    client_destroy(&c2);
    client_destroy(&c3);
}

/* returns the number of topics the subscriber is subscribed
 * to. 0 if it does not exist */
static int nsubs(struct list *list, struct subscriber *sub) {
    struct node *topics = list->root;
    int nsubs = 0;
    for (; topics != NULL; topics = topics->next) {
        struct topic *topic = topics->entry;
        struct node *cur = topic->subscribers->root;
        for (;cur != NULL; cur = cur->next) {
           struct subscriber *cursub = cur->entry;
           if (cursub == sub) nsubs++;
        }           
    }
    return nsubs;
}

static struct message *message_find_by_content(struct list *messages,
        char *content) {

    struct node *node = messages->root;
    for (; node != NULL; node = node->next) {
        struct message *msg = node->entry;
        if (strcmp(msg->content, content) == 0)
            return msg;
    }
    return NULL;
}

void test_topic_add_subscriber() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    struct subscriber sub3 = {&c3, "marta"};

    ret = topic_add_subscriber(&ts, "stocks", &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub3));

    ret = topic_add_subscriber(&ts, "stocks", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub3));

    ret = topic_add_subscriber(&ts, "bounds", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub3));

    ret = topic_add_subscriber(&ts, "stocks", &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub3));
}

void test_topic_init_and_destroy() {
    int ret;

    struct topic t;
    struct subscriber sub;
    
    ret = topic_init(&t);   
    CU_ASSERT_EQUAL_FATAL(0, ret);

    // would fail if list of subscribers
    // was not correctly initialized
    ret = list_add(t.subscribers, &sub);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    ret = topic_destroy(&t);   
    CU_ASSERT_PTR_NULL_FATAL(t.name);
    CU_ASSERT_PTR_NULL_FATAL(t.subscribers);
}

void test_topic_remove_subscriber() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    struct subscriber sub3 = {&c3, "marta"};
    topic_add_subscriber(&ts, "stocks", &sub1);
    topic_add_subscriber(&ts, "stocks", &sub2);
    topic_add_subscriber(&ts, "bounds", &sub2);
    topic_add_subscriber(&ts, "stocks", &sub3);

    ret = topic_remove_subscriber(&ts, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub3));

    ret = topic_remove_subscriber(&ts, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, &sub3));

    ret = topic_remove_subscriber(&ts, &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, &sub3));

    // topics are not removed
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
}

void test_add_message_1() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {&c1, "hans"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);

    // single message
    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&messages));
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(1, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);

    topic_after_test();
}

void test_add_message_2() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_subscriber(&topics, "stocks", &sub2);

    // single message
    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&messages));
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(2, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);
    msgstats = stats->root->next->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub2, msgstats->subscriber);
    topic_after_test();
}

void test_add_message_3() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_subscriber(&topics, "stocks", &sub2);

    // two messages
    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    ret = topic_add_message(&topics, &messages, "stocks", "price: 34");
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&messages));

    // first msg
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(2, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);
    msgstats = stats->root->next->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub2, msgstats->subscriber);

    // second msg
    msg = message_find_by_content(&messages, "price: 34");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(2, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);
    msgstats = stats->root->next->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub2, msgstats->subscriber);
    topic_after_test();
}


void test_add_message_late_subscriber() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);

    // send first message
    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&messages));

    // first msg: only sub1
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(1, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);

    // snd msg: both
    topic_add_subscriber(&topics, "stocks", &sub2);
    ret = topic_add_message(&topics, &messages, "stocks", "price: 34");
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&messages));
    // first still only has one sub
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(1, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);
    // second has two subs
    msg = message_find_by_content(&messages, "price: 34");
    CU_ASSERT_PTR_NOT_NULL_FATAL(msg);
    CU_ASSERT_STRING_EQUAL_FATAL("stocks", msg->topicname);
    stats = msg->stats;
    CU_ASSERT_EQUAL_FATAL(2, list_len(stats));
    msgstats = stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub1, msgstats->subscriber);
    msgstats = stats->root->next->entry;
    CU_ASSERT_EQUAL_FATAL(0, msgstats->last_fail);
    CU_ASSERT_EQUAL_FATAL(0, msgstats->nattempts);
    CU_ASSERT_EQUAL_FATAL(&sub2, msgstats->subscriber);
    topic_after_test();
}

void test_add_message_dead_subscriber() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);

    // set client 1 to dead, should not be copied to
    // statistics for message anymore
    c1.dead = 1;

    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(TOPIC_NO_SUBSCRIBERS, ret);
    topic_after_test();
}

void test_add_message_dead_subscriber_2() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_subscriber(&topics, "stocks", &sub2);

    // set client 1 to dead, should not be copied to
    // statistics for message anymore
    c1.dead = 1;

    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    assert(ret == 0);
    msg = messages.root->entry;
    CU_ASSERT_EQUAL_FATAL(1, list_len(msg->stats));
    msgstats = msg->stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(&sub2, msgstats->subscriber);
    topic_after_test();
}


void test_add_message_5() {
    int ret;
    struct list topics;
    struct list messages;
    list_init(&topics);
    list_init(&messages);
    // inexistent topic
    ret = topic_add_message(&topics, &messages, "foo", "price: 33");
    CU_ASSERT_EQUAL_FATAL(TOPIC_NOT_FOUND, ret);
}

void test_add_message_no_subscriber() {
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};
    list_init(&topics);
    list_init(&messages);
    // add and remove sub to create topic
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_remove_subscriber(&topics, &sub1);

    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(TOPIC_NO_SUBSCRIBERS, ret);
}

void test_msg_remove_subscriber_first() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    struct message *msg;
    struct msg_statistics *stat;
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_subscriber(&topics, "stocks", &sub2);
    topic_add_message(&topics, &messages, "stocks", "price: 33");

    ret = message_remove_subscriber(&messages, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_EQUAL_FATAL(1, list_len(msg->stats));
    stat = msg->stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(&sub2, stat->subscriber);
    topic_after_test();
}

void test_msg_remove_subscriber_second() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    struct message *msg;
    struct msg_statistics *stat;
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_subscriber(&topics, "stocks", &sub2);
    topic_add_message(&topics, &messages, "stocks", "price: 33");

    ret = message_remove_subscriber(&messages, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_EQUAL_FATAL(1, list_len(msg->stats));
    stat = msg->stats->root->entry;
    CU_ASSERT_EQUAL_FATAL(&sub1, stat->subscriber);
    topic_after_test();
}

void test_msg_remove_subscriber_last() {
    topic_before_test();
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};

    // one message
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");
    ret = message_remove_subscriber(&messages, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    // two messages
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");
    topic_add_message(&topics, &messages, "stocks", "price: 34");
    ret = message_remove_subscriber(&messages, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    topic_after_test();
}

void test_msg_remove_subscriber_not_subscribed() {
    topic_before_test();
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {&c1, "hans"};
    struct subscriber sub2 = {&c2, "jakob"};
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");

    int ret = message_remove_subscriber(&messages, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    topic_after_test();
}

void test_topic_strerror() {
    char buf[32];
    topic_strerror(TOPIC_NOT_FOUND, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("TOPIC_NOT_FOUND", buf);

    topic_strerror(TOPIC_NO_SUBSCRIBERS, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("TOPIC_NO_SUBSCRIBERS", buf);

    topic_strerror(-1, buf);
    CU_ASSERT_STRING_EQUAL_FATAL("UNKNOWN_ERROR", buf);

}

void topic_add_topic_suite() {
    CU_pSuite topicSuite = CU_add_suite("topic", NULL, NULL);
    CU_add_test(topicSuite, "test_topic_add_subscriber",
        test_topic_add_subscriber);
    CU_add_test(topicSuite, "test_topic_remove_subscriber",
        test_topic_remove_subscriber);
    CU_add_test(topicSuite, "test_add_message_1", test_add_message_1);
    CU_add_test(topicSuite, "test_add_message_2", test_add_message_2);
    CU_add_test(topicSuite, "test_add_message_3", test_add_message_3);
    CU_add_test(topicSuite, "test_add_message_late_subscriber",
        test_add_message_late_subscriber);
    CU_add_test(topicSuite, "test_add_message_5", test_add_message_5);
    CU_add_test(topicSuite, "test_add_message_no_subscriber",
        test_add_message_no_subscriber);
    CU_add_test(topicSuite, "test_add_message_dead_subscriber",
        test_add_message_dead_subscriber);
    CU_add_test(topicSuite, "test_add_message_dead_subscriber_2",
        test_add_message_dead_subscriber_2);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_first",
        test_msg_remove_subscriber_first);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_second",
        test_msg_remove_subscriber_second);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_last",
        test_msg_remove_subscriber_last);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_not_subscribed",
        test_msg_remove_subscriber_not_subscribed);
    CU_add_test(topicSuite, "test_topic_init_and_destroy",
        test_topic_init_and_destroy);
    CU_add_test(topicSuite, "test_topic_strerror",
        test_topic_strerror);
}
