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
    struct msg_statistics stat;

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

    stat.nattempts = 3;
    stat.last_fail = timestamp() - 1;
    CU_ASSERT_EQUAL_FATAL(1, gc_eligible_stat(&stat));   

    // never tried
    stat.nattempts = 0;
    stat.last_fail = 0;
    CU_ASSERT_EQUAL_FATAL(0, gc_eligible_stat(&stat));   

    // not enough attempts
    stat.nattempts = MAX_ATTEMPTS - 1;
    stat.last_fail = 4444;
    CU_ASSERT_EQUAL_FATAL(0, gc_eligible_stat(&stat));   
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
    struct subscriber sub;
    struct client client;

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
    client_init(&client);

    // eligible
    stat11.nattempts = MAX_ATTEMPTS;
    stat11.last_fail = timestamp();

    // not eligible
    stat12.nattempts = 1;
    stat12.last_fail = timestamp();

    // eligible (dead client)
    stat13.nattempts = 0;
    stat13.last_fail = 0;
    stat13.subscriber = &sub;
    sub.client = &client;
    client.dead = 1;

    ret = gc_collect_eligible_stats(&messages, &eligible);
    assert(ret == 0);

    void *entry1 = eligible.root->entry;
    void *entry2 = eligible.root->next->entry;

    CU_ASSERT(entry1 == &stat11 || entry2 == &stat11);
    CU_ASSERT(entry1 == &stat12 || entry2 == &stat12);
    CU_ASSERT(entry1 != entry2);

    client_destroy(&client);
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
    assert(ret == 0);

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

    struct list stats1;
    struct list stats2;
    struct list stats3;
    struct list stats4;
    struct list stats5;
    
    list_init(&messages);   
    list_init(&eligible);
    list_add(&messages, &msg1);
    list_add(&messages, &msg2);
    list_add(&messages, &msg3);
    list_add(&messages, &msg4);
    list_add(&messages, &msg5);
    list_init(&stats1);
    list_init(&stats2);
    list_init(&stats3);
    list_init(&stats4);
    list_init(&stats5);
    msg1.stats = &stats1;
    msg2.stats = &stats2;
    msg3.stats = &stats3;
    msg4.stats = &stats4;
    msg5.stats = &stats5;

    list_add(&eligible, &msg1);
    list_add(&eligible, &msg3);

    ret = gc_remove_eligible_msgs(&messages, &eligible);
    assert(ret == 0);

    CU_ASSERT_PTR_EQUAL_FATAL(eligible.root->entry, &msg2);
    CU_ASSERT_PTR_EQUAL_FATAL(eligible.root->next->entry, &msg4);
    CU_ASSERT_PTR_EQUAL_FATAL(eligible.root->next->next->entry, &msg5);
    CU_ASSERT_PTR_NULL_FATAL(eligible.root->next->next->next);

    // make sure stuff has been freed 
    CU_ASSERT_PTR_NULL_FATAL(msg1.content);
    CU_ASSERT_PTR_NULL_FATAL(msg1.topicname);
    CU_ASSERT_PTR_NULL_FATAL(msg1.stats);
    CU_ASSERT_PTR_NULL_FATAL(msg2.content);
    CU_ASSERT_PTR_NULL_FATAL(msg2.topicname);
    CU_ASSERT_PTR_NULL_FATAL(msg2.stats);

    list_destroy(&messages);
    list_destroy(&eligible);
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
    CU_ASSERT_PTR_NULL_FATAL(stat11.subscriber);
    CU_ASSERT_PTR_NULL_FATAL(stat21.statrwlock);
    CU_ASSERT_PTR_NULL_FATAL(stat21.subscriber);

    list_destroy(&messages);
    list_destroy(&eligible);
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
