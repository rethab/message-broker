#include <assert.h>
#include <pthread.h>

#include "gc.h"

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

        pthread_rwlock_t *deadrwlock;
        deadrwlock = stat->subscriber->client->deadrwlock; 

        // acquire read lock for dead flag
        ret = pthread_rwlock_rdlock(deadrwlock);
        assert(ret == 0);

        if (stat->subscriber->client->dead) {
            val = 1;
        }

        // release read lock for dead flag
        ret = pthread_rwlock_unlock(deadrwlock);
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
