#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "util.c"
#include "../src/topic.h"

/** TEST HELPER FUNCTIONS **/
static void debug_print_topic(struct list *list) {
    struct node *topics = list->root;
    printf("** Debug Print Topics: **\n");   
    for (; topics != NULL; topics = topics->next) {
        struct topic *topic = topics->entry;
        printf("   Topic Name: %s\n", topic->name);
        printf("   Topic Subscribers:\n");

        struct node *cur = topic->subscribers->root;
        for (;cur != NULL; cur = cur->next) {
           struct subscriber *sub = cur->entry;
           printf("      ID: %d, Name: %s, Sockfd: %d\n",
                sub->id, sub->name, sub->sockfd);
        }           
    }
}

/* returns the number of topics the subscriber is subscribed
 * to. 0 if it does not exist */
static int nsubs(struct list *list, int subscriberid) {
    struct node *topics = list->root;
    int nsubs = 0;
    for (; topics != NULL; topics = topics->next) {
        struct topic *topic = topics->entry;
        struct node *cur = topic->subscribers->root;
        for (;cur != NULL; cur = cur->next) {
           struct subscriber *sub = cur->entry;
           if (sub->id == subscriberid) nsubs++;
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

/* returns the number of elements in a list */
static int list_len(struct list *list) {
    struct node *cur = list->root;
    int n = 0;
    for (; cur != NULL; cur = cur->next) {
        n++;
    }
    return n;
}

/* checks whether an entry exists */
static int list_exists(struct list *list, void *entry) {
    struct node *cur = list->root;
    for (; cur != NULL; cur = cur->next) {
        if (cur->entry == entry) return 1;
    }
    return 0;
}

/* TESTS */

void test_list_len() {
    char a;
    struct list list;

    // delete in the middle
    list_init(&list);
    list_add(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, list_empty(&list));

    list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(1, list_empty(&list));

    list_init(&list);
    CU_ASSERT_EQUAL_FATAL(1, list_empty(&list));
}

void test_add_remove_list() {
    char a,b,c,d;
    int ret;
    struct list list;

    // delete in the middle
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &b);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete last
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete first
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // delete all
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    ret = list_remove(&list, &a);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));
    ret = list_remove(&list, &b);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(1, list_len(&list));
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(0, list_len(&list));

    // delete two in the middle
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    list_add(&list, &c);
    list_add(&list, &d);
    ret = list_remove(&list, &b);
    ret = list_remove(&list, &c);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &c));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &d));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));

    // remove inexistent
    list_init(&list);
    list_add(&list, &a);
    list_add(&list, &b);
    ret = list_remove(&list, &d);
    CU_ASSERT_EQUAL_FATAL(LIST_NOT_FOUND, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &a));
    CU_ASSERT_EQUAL_FATAL(1, list_exists(&list, &b));
    CU_ASSERT_EQUAL_FATAL(0, list_exists(&list, &d));
    CU_ASSERT_EQUAL_FATAL(2, list_len(&list));
} 

void test_topic_add_subscriber() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};

    ret = topic_add_subscriber(&ts, "stocks", &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = topic_add_subscriber(&ts, "stocks", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = topic_add_subscriber(&ts, "bounds", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = topic_add_subscriber(&ts, "stocks", &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));
}

void test_topic_remove_subscriber() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};
    topic_add_subscriber(&ts, "stocks", &sub1);
    topic_add_subscriber(&ts, "stocks", &sub2);
    topic_add_subscriber(&ts, "bounds", &sub2);
    topic_add_subscriber(&ts, "stocks", &sub3);

    ret = topic_remove_subscriber(&ts, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = topic_remove_subscriber(&ts, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = topic_remove_subscriber(&ts, &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    // topics are not removed
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
}

void test_add_message_1() {
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {1, 10, "hans"};
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
}

void test_add_message_2() {
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
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
}

void test_add_message_3() {
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
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
}


void test_add_message_late_subscriber() {
    int ret;
    struct list topics;
    struct list messages;
    struct message *msg;
    struct list *stats;
    struct msg_statistics *msgstats;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    list_init(&topics);
    list_init(&messages);

    topic_add_subscriber(&topics, "stocks", &sub1);

    // subscriber gets messages that were sent before
    // his arrival
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
    struct subscriber sub1 = {1, 10, "hans"};
    list_init(&topics);
    list_init(&messages);
    // add and remove sub to create topic
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_remove_subscriber(&topics, &sub1);

    ret = topic_add_message(&topics, &messages, "stocks", "price: 33");
    CU_ASSERT_EQUAL_FATAL(TOPIC_NO_SUBSCRIBERS, ret);
}

void test_msg_remove_subscriber_first() {
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
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
}

void test_msg_remove_subscriber_second() {
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
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
}

void test_msg_remove_subscriber_last() {
    int ret;
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {1, 10, "hans"};
    struct message *msg;

    // one message
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");
    ret = message_remove_subscriber(&messages, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NULL_FATAL(msg);
    CU_ASSERT_EQUAL_FATAL(0, list_len(&messages));

    // two messages
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");
    topic_add_message(&topics, &messages, "stocks", "price: 34");
    ret = message_remove_subscriber(&messages, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    msg = message_find_by_content(&messages, "price: 33");
    CU_ASSERT_PTR_NULL_FATAL(msg);
    msg = message_find_by_content(&messages, "price: 34");
    CU_ASSERT_PTR_NULL_FATAL(msg);
    CU_ASSERT_EQUAL_FATAL(0, list_len(&messages));
}

void test_msg_remove_subscriber_not_subscribed() {
    struct list topics;
    struct list messages;
    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    list_init(&topics);
    list_init(&messages);
    topic_add_subscriber(&topics, "stocks", &sub1);
    topic_add_message(&topics, &messages, "stocks", "price: 33");

    int ret = message_remove_subscriber(&messages, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
}

int main(int argc, char** argv) {
    install_segfault_handler();

    assert(CUE_SUCCESS == CU_initialize_registry());

    CU_pSuite listSuite = CU_add_suite("list", NULL, NULL);
    CU_add_test(listSuite, "test_add_remove_list",
        test_add_remove_list);
    CU_add_test(listSuite, "test_list_len",
        test_list_len);

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
    CU_add_test(topicSuite, "test_msg_remove_subscriber_first",
        test_msg_remove_subscriber_first);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_second",
        test_msg_remove_subscriber_second);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_last",
        test_msg_remove_subscriber_last);
    CU_add_test(topicSuite, "test_msg_remove_subscriber_not_subscribed",
        test_msg_remove_subscriber_not_subscribed);

    CU_basic_run_tests();

    CU_cleanup_registry();
    exit(EXIT_SUCCESS);
}
