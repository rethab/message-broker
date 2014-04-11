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

void test_add_subscriber_to_topic() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};

    ret = add_subscriber_to_topic(&ts, "stocks", &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "stocks", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "bounds", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "stocks", &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));
}

void test_remove_subscriber() {
    int ret;
    struct list ts;

    list_init(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};
    add_subscriber_to_topic(&ts, "stocks", &sub1);
    add_subscriber_to_topic(&ts, "stocks", &sub2);
    add_subscriber_to_topic(&ts, "bounds", &sub2);
    add_subscriber_to_topic(&ts, "stocks", &sub3);

    ret = remove_subscriber(&ts, &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = remove_subscriber(&ts, &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = remove_subscriber(&ts, &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    // topics are not removed
    CU_ASSERT_EQUAL_FATAL(2, list_len(&ts));
}

int main(int argc, char** argv) {
    install_segfault_handler();

    assert(CUE_SUCCESS == CU_initialize_registry());

    CU_pSuite listSuite = CU_add_suite("list", NULL, NULL);
    CU_add_test(listSuite, "test_add_remove_list",
        test_add_remove_list);

    CU_pSuite topicSuite = CU_add_suite("topic", NULL, NULL);
    CU_add_test(topicSuite, "test_add_subscriber_to_topic",
        test_add_subscriber_to_topic);
    CU_add_test(topicSuite, "test_remove_subscriber",
        test_remove_subscriber);

    CU_basic_run_tests();

    CU_cleanup_registry();
    exit(EXIT_SUCCESS);
}
