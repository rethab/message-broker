#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "util.c"
#include "../src/topic.h"

/** TEST HELPER FUNCTIONS **/
static void debug_print_topic(struct list *topics) {
    printf("** Debug Print Topics: **\n");   
    for (; topics != NULL; topics = topics->next) {
        struct topic *topic = topics->entry;
        printf("   Topic Name: %s\n", topic->name);
        printf("   Topic Subscribers:\n");

        struct list *cur = topic->subscribers;
        for (;cur != NULL; cur = cur->next) {
           struct subscriber *sub = cur->entry;
           printf("      ID: %d, Name: %s, Sockfd: %d\n",
                sub->id, sub->name, sub->sockfd);
        }           
    }
}

/* returns the number of topics the subscriber is subscribed
 * to. 0 if it does not exist */
static int nsubs(struct list *topics, int subscriberid) {
    int nsubs = 0;
    for (; topics != NULL; topics = topics->next) {
        struct topic *topic = topics->entry;
        struct list *cur = topic->subscribers;
        for (;cur != NULL; cur = cur->next) {
           struct subscriber *sub = cur->entry;
           if (sub->id == subscriberid) nsubs++;
        }           
    }
    return nsubs;
}

/* returns the number of topics */
static int ntopics(struct list *topics) {
    int n = 0;
    for (; topics != NULL; topics = topics->next) {
        n++;
    }
    return n;
}

void test_add_subscriber_to_topic() {
    int ret;
    struct list ts;

    init_list(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};

    ret = add_subscriber_to_topic(&ts, "stocks", &sub1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, ntopics(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "stocks", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, ntopics(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "bounds", &sub2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, ntopics(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    ret = add_subscriber_to_topic(&ts, "stocks", &sub3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(2, ntopics(&ts));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(2, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));
}

void test_remove_subscriber() {
    int ret;
    struct list ts;

    init_list(&ts);

    struct subscriber sub1 = {1, 10, "hans"};
    struct subscriber sub2 = {2, 20, "jakob"};
    struct subscriber sub3 = {3, 30, "marta"};
    add_subscriber_to_topic(&ts, "stocks", &sub1);
    add_subscriber_to_topic(&ts, "stocks", &sub2);
    add_subscriber_to_topic(&ts, "bounds", &sub2);
    add_subscriber_to_topic(&ts, "stocks", &sub3);

    ret = remove_subscriber(&ts, 2);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = remove_subscriber(&ts, 1);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(1, nsubs(&ts, 3));

    ret = remove_subscriber(&ts, 3);
    CU_ASSERT_EQUAL_FATAL(0, ret);
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 1));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 2));
    CU_ASSERT_EQUAL_FATAL(0, nsubs(&ts, 3));

    // topics are not removed
    CU_ASSERT_EQUAL_FATAL(2, ntopics(&ts));
}

int main(int argc, char** argv) {
    install_segfault_handler();

    assert(CUE_SUCCESS == CU_initialize_registry());

    CU_pSuite parseSuite = CU_add_suite("topic", NULL, NULL);
    CU_add_test(parseSuite, "test_add_subscriber_to_topic",
        test_add_subscriber_to_topic);
    CU_add_test(parseSuite, "test_remove_subscriber",
        test_remove_subscriber);

    CU_basic_run_tests();

    CU_cleanup_registry();
    exit(EXIT_SUCCESS);
}
