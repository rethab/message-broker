#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#include "gc.h"
#include "broker.h"

void *gc_main_loop(void *arg) {
    struct broker_context *ctx = arg;

    int ret;

    while (1) {

        ret = gc_run_gc(ctx);
        assert(ret == 0);

        sleep(GC_PASS_TIMEOUT);
    }

    return 0;
}

int gc_run_gc(struct broker_context *ctx) {
    int ret;

    struct list messages;
    struct list stats;

    list_init(&stats);
    list_init(&messages);

    // collect and remove statistic
    ret = gc_collect_eligible_stats(ctx->messages, &stats);
    assert(ret == 0);

    ret = gc_remove_eligible_stats(ctx->messages, &stats);
    assert(ret == 0);

    ret = list_len(&stats);
    printf("GC: Removed %d Statistics\n", ret);

    // collect and remove messages
    ret = gc_collect_eligible_msgs(ctx->messages, &messages);
    assert(ret == 0);

    ret = gc_remove_eligible_msgs(ctx->messages, &messages);
    assert(ret == 0);

    ret = list_len(&stats);
    printf("GC: Removed %d Messages\n", ret);

    list_clean(&stats);
    list_clean(&messages);

    list_destroy(&stats);
    list_destroy(&messages);

    return 0;
}


int gc_eligible_stat(struct msg_statistics *stat) {

    int ret; // return value from other functions
    int val = 0;

    if (stat->last_fail == 0 && stat->nattempts > 0) {
        // sent successfully
        val = 1;
    } else if (stat->nattempts >= MAX_ATTEMPTS) {
        // too many attempts already
        val = 1;
    } else {

        pthread_mutex_t *deadmutex;
        deadmutex = stat->subscriber->client->deadmutex; 

        // acquire read lock for dead flag
        ret = pthread_mutex_lock(deadmutex);
        assert(ret == 0);

        if (stat->subscriber->client->dead) {
            val = 1;
        }

        // release read lock for dead flag
        ret = pthread_mutex_unlock(deadmutex);
        assert(ret == 0);
    }

    return val;
}

int gc_eligible_msg(struct message *msg) {

    int ret; // return value from other functions
    int val;

    // acquire read lock for statstics list
    ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
    assert(ret == 0);

    val = (msg->stats->root == NULL);

    // release read lock for statstics list
    ret = pthread_rwlock_unlock(msg->stats->listrwlock);
    assert(ret == 0);

    return val;
}

int gc_collect_eligible_stats(struct list *messages,
                              struct list *eligible) {

    int ret;

    // acquire read lock for messages list
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *curMsg = messages->root;
    while (curMsg != NULL) {

        struct message *msg = curMsg->entry;

        // acquire read lock for stats list
        ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
        assert(ret == 0);

        struct node *curStat = msg->stats->root;
        while (curStat != NULL) {
            if (gc_eligible_stat(curStat->entry)) {
                  ret = list_add(eligible, curStat->entry);  
                  assert(ret == 0);
            }
            curStat = curStat->next;
        }

        // release read lock for stats list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);

        curMsg = curMsg->next;
    }

    // release read lock for messages list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}

int gc_collect_eligible_msgs(struct list *messages,
                             struct list *eligible) {

    int ret;

    // acquire read lock for messages list
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *curMsg = messages->root;
    while (curMsg != NULL) {

        struct message *msg = curMsg->entry;

        // acquire read lock for stats list
        ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
        assert(ret == 0);

        if (gc_eligible_msg(msg)) {
            ret = list_add(eligible, msg);   
            assert(ret == 0);
        }

        // release read lock for stats list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);

        curMsg = curMsg->next;
    }

    // release read lock for messages list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}

int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible) {
    int ret;

    // acquire read lock on message list
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *curStat = eligible->root;
    while (curStat != NULL) {

        struct node *curMsg = messages->root;
        while (curMsg != NULL) {
            struct message *msg = curMsg->entry;

            // acquire write lock on statistic
            ret = pthread_rwlock_wrlock(msg->stats->listrwlock);
            assert(ret == 0);

            ret = list_remove(msg->stats, curStat->entry);
            if (ret == 0) {
                msg_statistics_destroy(curStat->entry);
            } else {
                assert(ret == LIST_NOT_FOUND);
            }

            // release write lock on statistic
            ret = pthread_rwlock_unlock(msg->stats->listrwlock);
            assert(ret == 0);


            curMsg = curMsg->next;
        }

        curStat = curStat->next;
    }

    // release write lock on message list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}

int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible) {
    int ret;

    // acquire write lock on message list
    ret = pthread_rwlock_wrlock(messages->listrwlock);
    assert(ret == 0);

    struct node *cur = eligible->root;
    while (cur != NULL) {

        ret = list_remove(messages, cur->entry);
        if (ret == 0) {
            message_destroy(cur->entry);
        } else {
            assert(ret == LIST_NOT_FOUND);
        }

        cur = cur->next;
    }

    // release write lock on message list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}
