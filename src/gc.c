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
    /* the messages, statistics and subscribers are
     * cleaned up in two steps: collect a list of
     * eligible entries and then remove those
     * that have been collected.
     *
     * note that there is no synchronization between the
     * two and thus one could think that the list might
     * change between collecting and removing (e.g.
     * a message was considered eligible and in the second
     * step it is no longer eligible). However, this case is not
     * possible by design! The eligibility of any of the
     * three types moves only from 'no' to 'yes', which means
     * once something is considered eligible, there is
     * no way back. reasoning:
     * 1. message: a message's list of subscribers is copied
     *             at creation time not additional subsribers
     *             are added along the way. therefore, once
     *             is is empty, it doesn't grow anymore.
     * 2. msg_statistics:
     *      I.   MAX_ATTEMPTS reached: the number of attempts
     *              can only grow.
     *      II.  client dead: there is no way back once the
     *              client is dead
     *      III. successfyl delivery: the message can only
     *              be delivered once.
     * 3. subscriber: a subscriber is eligible when it is
     *                dead and no statistics point to it.
     *                --> once a subscriber is considered dead,
     *                it is not copied from the topic to the
     *                message anymore.
     * */
    int ret;


    // collect and remove statistic
    struct list stats;
    ret = list_init(&stats);
    assert(ret == 0);
    ret = gc_collect_eligible_stats(ctx->messages, &stats);
    assert(ret == 0);
    ret = gc_remove_eligible_stats(ctx->messages, &stats);
    assert(ret >= 0);
    if (ret != 0) fprintf(stderr, "GC: Removed %d Statistics\n", ret);
    ret = list_clean(&stats);
    assert(ret == 0);
    ret = list_destroy(&stats);
    assert(ret == 0);


    // collect and remove messages
    struct list messages;
    ret = list_init(&messages);
    assert(ret == 0);
    ret = gc_collect_eligible_msgs(ctx->messages, &messages);
    assert(ret == 0);
    ret = gc_remove_eligible_msgs(ctx->messages, &messages);
    assert(ret >= 0);
    if (ret != 0) fprintf(stderr, "GC: Removed %d Messages\n", ret);
    ret = list_clean(&messages);
    assert(ret == 0);
    ret = list_destroy(&messages);
    assert(ret == 0);


    // collect and remove subscribers
    struct list subscribers;
    ret = list_init(&subscribers);
    assert(ret == 0);
    ret = gc_collect_eligible_subscribers(ctx->topics, ctx->messages,
        &subscribers);
    assert(ret == 0);
    if (!list_empty(&subscribers)) {
        ret = gc_remove_eligible_subscribers(ctx->topics, &subscribers);
        assert(ret == 0);
        ret = gc_destroy_subscribers(&subscribers);
        assert(ret == 0);
        printf("GC: Removed %d Subscribers\n", list_len(&subscribers));
    }
    ret = list_clean(&subscribers);
    assert(ret == 0);
    ret = list_destroy(&subscribers);
    assert(ret == 0);

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
            struct msg_statistics *stat = curStat->entry;

            // acquire read lock for statistics
            ret = pthread_rwlock_rdlock(stat->statrwlock);
            assert(ret == 0);

            if (gc_eligible_stat(stat)) {
                  ret = list_add(eligible, stat);  
                  assert(ret == 0);
            }

            // release read lock for statistics
            ret = pthread_rwlock_unlock(stat->statrwlock);
            assert(ret == 0);

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

int gc_collect_eligible_subscribers(struct list *topics,
                                    struct list *messages,
                                    struct list *eligible) {
    /* find dead subscribers via topics and then
     * make sure each of those has no statistics */

    int ret;

    // acquire read lock on topic list
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);

    struct node *curTopic = topics->root;
    while (curTopic != NULL) {
        struct topic *topic = curTopic->entry;
        
        // acquire read lock on subscribers list
        ret = pthread_rwlock_rdlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        struct node *curSub = topic->subscribers->root;
        while (curSub != NULL) {
            struct subscriber *sub = curSub->entry;
            int dead;

            // acquire dead flag lock
            ret = pthread_mutex_lock(sub->client->deadmutex);
            assert(ret == 0);

            dead = sub->client->dead;           
            
            // release dead flag lock
            ret = pthread_mutex_unlock(sub->client->deadmutex);
            assert(ret == 0);

            // list_contains: subscriber could be subscribed
            // to other topic as well
            if (dead && !list_contains(eligible, sub)) {
                ret = list_add(eligible, sub);
                assert(ret == 0);
            }

            curSub = curSub->next;
        }

        // release read lock on subscribers list
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        curTopic = curTopic->next;
    }

    // release read lock on topic list
    ret = pthread_rwlock_unlock(topics->listrwlock);
    assert(ret == 0);

    // early exit, if none are dead, there's
    // nothing to verify
    if (list_empty(eligible)) {
        return 0;
    }

    // acquire read lock on messages
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *curMsg = messages->root;
    while (curMsg != NULL) {
        struct message *msg = curMsg->entry;

        // acquire read lock on statistics list
        ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
        assert(ret == 0);

        struct node *curStat = msg->stats->root;
        while (curStat != NULL) {
            struct msg_statistics *stat = curStat->entry;

            struct node *curSub = eligible->root;
            while (curSub != NULL) {
                struct subscriber *sub = curSub->entry;

                // there still is some statistic for
                // this subscriber. gc will clean
                // this up in the next pass
                if (sub == stat->subscriber) {
                    ret = list_remove(eligible, sub);   
                    assert(ret == 0);
                }
                
                curSub = curSub->next;
            }

            curStat = curStat->next;
        }

        // release read lock on statistics list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);
        
        curMsg = curMsg->next;
    }

    // release read lock on messages
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}

int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible) {
    int ret;

    int nstats = 0;

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
                nstats++;
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

    return nstats;
}

int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible) {
    int ret;

    int nmsgs = 0;

    // acquire write lock on message list
    ret = pthread_rwlock_wrlock(messages->listrwlock);
    assert(ret == 0);

    struct node *cur = eligible->root;
    while (cur != NULL) {

        ret = list_remove(messages, cur->entry);
        if (ret == 0) {
            message_destroy(cur->entry);
            nmsgs++;
        } else {
            assert(ret == LIST_NOT_FOUND);
        }

        cur = cur->next;
    }

    // release write lock on message list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return nmsgs;
}

int gc_remove_eligible_subscribers(struct list *topics,
                                   struct list *eligible) {
    
    int ret;

    // acquire read lock on topics
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);

    struct node *curTopic = topics->root;
    while (curTopic != NULL) {
        struct topic *topic = curTopic->entry;

        // this topic contains subscribers to remove
        int has_subs;

        // acquire read lock on subscribers to check
        ret = pthread_rwlock_rdlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        struct node *curSub = topic->subscribers->root;
        while (curSub != NULL) {
            struct subscriber *sub = curSub->entry;

            if (list_contains(eligible, sub)) {
                has_subs = 1;
                break;
            } else {
                has_subs = 0;
            }

            curSub = curSub->next;
        }

        // release read lock on subscribers
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        // now we need the write lock to actually remove them
        if (has_subs) {

            // acquire write lock to remove subscribers
            ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
            assert(ret == 0);

            struct node *curSub = eligible->root;
            while (curSub != NULL) {
                struct subscriber *sub = curSub->entry;

                ret = list_remove(topic->subscribers, sub);
                // not found is ok, as we are trying all
                assert(ret == 0 || ret == LIST_NOT_FOUND);

                curSub = curSub->next;
            }

            // release write lock on subscribers
            ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
            assert(ret == 0);
        }
        
        curTopic = curTopic->next;
    }

    // release read lock on topics
    ret = pthread_rwlock_unlock(topics->listrwlock);
    assert(ret == 0);

    return 0;
}

int gc_destroy_subscribers(struct list *subscribers) {
    struct node *curSub = subscribers->root;
    while (curSub != NULL) {
        struct subscriber *sub = curSub->entry;
        struct client *client = sub->client;

        client_destroy(client);
        free(client);
        sub->client = NULL;
        free(sub->name);
        sub->name = NULL;
        
        curSub = curSub->next;
    }

    return 0;
}
