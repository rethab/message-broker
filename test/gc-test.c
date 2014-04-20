#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "../src/gc.h"

static long timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void test_gc_eligible_stat() {
    struct client client;
    struct subscriber sub;
    struct msg_statistics stat;

    client_init(&client);
    stat.subscriber = &sub;
    sub.client = &client;


    // too many attempts
    stat.nattempts = MAX_ATTEMPTS;
    stat.last_fail = timestamp();
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_stat(&stat));   

    // successfully delivered
    stat.nattempts = 33;
    stat.last_fail = 0;
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_stat(&stat));   

    // successfully delivered
    stat.nattempts = MAX_ATTEMPTS;
    stat.last_fail = 0;
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_stat(&stat));   

    // never tried
    stat.nattempts = 0;
    stat.last_fail = 0;
    CU_ASSERT_EQUAL_FATAL(0, gc_eligible_stat(&stat));   

    // not enough attempts
    stat.nattempts = MAX_ATTEMPTS - 1;
    stat.last_fail = 4444;
    CU_ASSERT_EQUAL_FATAL(0, gc_eligible_stat(&stat));   

    // dead client
    stat.nattempts = 0;
    stat.last_fail = 0;
    client.dead = 1;
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_stat(&stat));   

}

void test_gc_eligible_msg() {
    struct list stats;
    struct message msg;
    struct msg_statistics stat;

    list_init(&stats);

    // empty stats list
    msg.stats = &stats;
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_msg(&msg));

    // non empty stats list
    msg.stats = &stats;
    list_add(&stats, &stat);
    CU_ASSERT_EQUAL_FATAL(0, gc_eligible_msg(&msg));
}

void test_gc_collect_eligible_stats() {
    int ret;
    struct list messages;
    struct list eligible;

    struct message msg1;
    struct list stats1;
    struct msg_statistics stat11;
    struct msg_statistics stat12;
    struct msg_statistics stat13;
    struct subscriber sub1;
    struct client client1;
    struct subscriber sub2;
    struct client client2;
    struct subscriber sub3;
    struct client client3;

    struct message msg2;
    struct list stats2;
    
    list_init(&messages);   
    list_init(&stats1);
    list_init(&stats2);
    list_init(&eligible);
    list_add(&messages, &msg1);
    list_add(&messages, &msg2);
    list_add(&stats1, &stat11);
    list_add(&stats1, &stat12);
    list_add(&stats1, &stat13);
    msg1.stats = &stats1;
    msg2.stats = &stats2;
    client_init(&client1);
    client_init(&client2);
    client_init(&client3);

    // eligible
    stat11.nattempts = MAX_ATTEMPTS;
    stat11.last_fail = timestamp();
    stat11.subscriber = &sub1;
    sub1.client = &client1;

    // not eligible
    stat12.nattempts = 1;
    stat12.last_fail = timestamp();
    stat12.subscriber = &sub2;
    sub2.client = &client2;

    // eligible (dead client)
    stat13.nattempts = 0;
    stat13.last_fail = 0;
    stat13.subscriber = &sub3;
    sub3.client = &client3;
    client3.dead = 1;

    ret = gc_collect_eligible_stats(&messages, &eligible);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    void *entry1 = eligible.root->entry;
    void *entry2 = eligible.root->next->entry;

    CU_ASSERT(entry1 == &stat11 || entry2 == &stat11);
    CU_ASSERT(entry1 == &stat13 || entry2 == &stat13);
    CU_ASSERT(entry1 != entry2);

    client_destroy(&client1);
}

void test_gc_collect_eligible_msgs() {
    int ret;
    struct list messages;
    struct list eligible;

    struct message msg1;
    struct list stats1;
    struct msg_statistics stat11;

    struct message msg2;
    struct list stats2;
    
    list_init(&messages);   
    list_init(&stats1);
    list_init(&stats2);
    list_init(&eligible);
    list_add(&messages, &msg1);
    list_add(&messages, &msg2);
    list_add(&stats1, &stat11);
    msg1.stats = &stats1;
    msg2.stats = &stats2;

    ret = gc_collect_eligible_msgs(&messages, &eligible);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    // msg1 is not eligible as it has stats
    CU_ASSERT_PTR_EQUAL_FATAL(eligible.root->entry, &msg2);
    CU_ASSERT_PTR_NULL_FATAL(eligible.root->next);
}

void test_gc_remove_eligible_msgs() {
    int ret;
    struct list messages;
    struct list eligible;

    struct message msg1;
    struct message msg2;
    struct message msg3;
    struct message msg4;
    struct message msg5;
    struct message msg6;

    struct list *stats1 = malloc(sizeof(struct list));
    struct list *stats2 = malloc(sizeof(struct list));
    struct list *stats3 = malloc(sizeof(struct list));
    struct list *stats4 = malloc(sizeof(struct list));
    struct list *stats5 = malloc(sizeof(struct list));
    struct list *stats6 = malloc(sizeof(struct list));
    
    list_init(&messages);   
    list_init(&eligible);
    list_add(&messages, &msg1);
    list_add(&messages, &msg2);
    list_add(&messages, &msg3);
    list_add(&messages, &msg4);
    list_add(&messages, &msg5);
    list_init(stats1);
    list_init(stats2);
    list_init(stats3);
    list_init(stats4);
    list_init(stats5);
    list_init(stats6);
    msg1.stats = stats1;
    msg1.content = NULL;
    msg1.topicname = NULL;
    msg2.stats = stats2;
    msg2.content = NULL;
    msg2.topicname = NULL;
    msg3.stats = stats3;
    msg3.content = NULL;
    msg3.topicname = NULL;
    msg4.stats = stats4;
    msg4.content = NULL;
    msg4.topicname = NULL;
    msg5.stats = stats5;
    msg5.content = NULL;
    msg5.topicname = NULL;

    list_add(&eligible, &msg1);
    list_add(&eligible, &msg3);
    list_add(&eligible, &msg6);

    ret = gc_remove_eligible_msgs(&messages, &eligible);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    CU_ASSERT_PTR_EQUAL_FATAL(messages.root->entry, &msg2);
    CU_ASSERT_PTR_EQUAL_FATAL(messages.root->next->entry, &msg4);
    CU_ASSERT_PTR_EQUAL_FATAL(messages.root->next->next->entry, &msg5);
    CU_ASSERT_PTR_NULL_FATAL(messages.root->next->next->next);

    // make sure stuff has been freed 
    CU_ASSERT_PTR_NULL_FATAL(msg1.content);
    CU_ASSERT_PTR_NULL_FATAL(msg1.topicname);
    CU_ASSERT_PTR_NULL_FATAL(msg1.stats);
    CU_ASSERT_PTR_NULL_FATAL(msg3.content);
    CU_ASSERT_PTR_NULL_FATAL(msg3.topicname);
    CU_ASSERT_PTR_NULL_FATAL(msg3.stats);
}

void test_gc_remove_eligible_stats() {
    int ret;
    struct list messages;
    struct list eligible;

    struct message msg1;
    struct list stats1;
    struct msg_statistics stat11;
    struct msg_statistics stat12;
    struct msg_statistics stat13;
    struct msg_statistics stat21;
    struct msg_statistics stat22;

    struct message msg2;
    struct list stats2;
    
    list_init(&messages);   
    list_init(&stats1);
    list_init(&stats2);
    list_init(&eligible);
    list_add(&messages, &msg1);
    list_add(&messages, &msg2);
    list_add(&stats1, &stat11);
    list_add(&stats1, &stat12);
    list_add(&stats1, &stat13);
    list_add(&stats2, &stat21);
    list_add(&stats2, &stat22);
    msg_statistics_init(&stat11);
    msg_statistics_init(&stat12);
    msg_statistics_init(&stat13);
    msg_statistics_init(&stat21);
    msg_statistics_init(&stat22);
    msg1.stats = &stats1;
    msg2.stats = &stats2;

    list_add(&eligible, &stat11);
    list_add(&eligible, &stat12);
    list_add(&eligible, &stat22);

    ret = gc_remove_eligible_stats(&messages, &eligible);
    CU_ASSERT_EQUAL_FATAL(0, ret);

    CU_ASSERT_PTR_EQUAL_FATAL(&stat13, msg1.stats->root->entry);
    CU_ASSERT_PTR_NULL_FATAL(msg1.stats->root->next);
    CU_ASSERT_PTR_EQUAL_FATAL(&stat21, msg2.stats->root->entry);
    CU_ASSERT_PTR_NULL_FATAL(msg2.stats->root->next);

    // explicitly not free subscriber
    CU_ASSERT_PTR_NULL_FATAL(stat11.statrwlock);
    CU_ASSERT_PTR_NULL_FATAL(stat12.statrwlock);
    CU_ASSERT_PTR_NULL_FATAL(stat22.statrwlock);

}

void gc_test_suite() {
    CU_pSuite gcSuite = CU_add_suite("gc", NULL, NULL);
    CU_add_test(gcSuite, "test_gc_eligible_stat",
        test_gc_eligible_stat); 
    CU_add_test(gcSuite, "test_gc_eligible_msg",
        test_gc_eligible_msg); 
    CU_add_test(gcSuite, "test_gc_collect_eligible_stats",
        test_gc_collect_eligible_stats); 
    CU_add_test(gcSuite, "test_gc_collect_eligible_msgs",
        test_gc_collect_eligible_msgs); 
    CU_add_test(gcSuite, "test_gc_remove_eligible_msgs",
        test_gc_remove_eligible_msgs); 
    CU_add_test(gcSuite, "test_gc_remove_eligible_stats",
        test_gc_remove_eligible_stats); 
}
