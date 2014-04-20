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
        ret = pthread_rwlock_rdlock(deadrwlock);
        assert(ret == 0);
    }

    return val;
}

int gc_eligible_msg(struct message *msg) {
    return msg->stats->root == NULL;
}

int gc_collect_eligible_stats(struct list *messages,
                              struct list *eligible) {
    return -1;
}

int gc_collect_eligible_msgs(struct list *messages,
                             struct list *eligible) {
    return -1;
}

int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible) {
    return -1;
}

int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible) {
    return -1;
}
