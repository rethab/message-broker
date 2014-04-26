#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "topic.h"

/* at least read lock for list of topics must be held by 
 * the funciton calling this function */
static struct topic *find_topic(struct list *topics, char *name) {
    struct node *cur = topics->root;
    while (cur != NULL) {
        struct topic *topic = cur->entry;
        if (strcmp(topic->name, name) == 0)
            return topic;
        else
            cur = cur->next;
    }
    return NULL;
}

/* write lock on topic list must be held */
static struct topic *create_new_topic(struct list *topics, char *name) {
    int ret;

    struct topic *topic = malloc(sizeof(struct topic));
    assert(topic != NULL);

    ret = topic_init(topic);
    assert(ret == 0);

    topic->name = strdup(name);

    ret = list_add(topics, topic);
    assert(ret == 0);
    return topic;
}

int topic_add_subscriber(struct list *topics, char *name,
                struct subscriber *subscriber) {

    int ret; // return values from other functions
    int val = 0; // return value for this function
    struct topic *topic;

    /* first try with readlock if the
     * topic exists. this will be faster in most
     * scenarios. if it doesn't exist, create it
     * with the write lock 
     */

    // acquire read lock for topics list
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);


    topic = find_topic(topics, name);

    if (topic != NULL) {

        /* we are still holding the read lock for the
         * topics list, which is enough to add to the
         * subscribers list
         */

        // acquire subscribers list write lock
        ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        ret = list_add(topic->subscribers, subscriber);
        assert(ret == 0);

        // release subscribers list write lock
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        // release read lock for topics list
        ret = pthread_rwlock_unlock(topics->listrwlock);
        assert(ret == 0);
    } else {

        /* topic does not exist --> release
        * readlock and acquire write lock for creation.
        * then check again whether it exists.
        */

        // release topics list read lock
        ret = pthread_rwlock_unlock(topics->listrwlock);
        assert(ret == 0);

        // acquire topics list write lock
        ret = pthread_rwlock_wrlock(topics->listrwlock);
        assert(ret == 0);

        // check existence again
        topic = find_topic(topics, name);

        if (topic == NULL) {
            topic = create_new_topic(topics, name);
        }

        // acquire subscribers list write lock
        ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        ret = list_add(topic->subscribers, subscriber);
        assert(ret == 0);

        // release subscribers list write lock
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        // release topics list write lock
        ret = pthread_rwlock_unlock(topics->listrwlock);
        assert(ret == 0);
    }

    return val;
}

int topic_remove_subscriber(struct list *topics,
            struct subscriber *subscriber) {

    int ret;

    // acquire read lock of topics
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);

    struct node *cur = topics->root;
    while (cur != NULL) {
        struct topic *topic = cur->entry;

        // acquire write lock of subscribers
        ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        ret = list_remove(topic->subscribers, subscriber);
        // not found is ok, since we just try
        assert(ret == LIST_NOT_FOUND || ret == 0);

        // release write lock of subscribers
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        cur = cur->next;
    }

    // release read lock of topics
    ret = pthread_rwlock_unlock(topics->listrwlock);
    assert(ret == 0);

    return 0;
}

static int subscriber_dead(struct subscriber *sub) {
    int ret;
    int dead;

    struct client *client = sub->client;

    // acquire read lock on dead flag
    ret = pthread_mutex_lock(client->deadmutex);
    assert(ret == 0);

    dead = client->dead;
    
    // release read lock on dead flag
    ret = pthread_mutex_unlock(client->deadmutex);
    assert(ret == 0);

    return dead;
}

static int count_alive_subscriber(struct list *subscribers) {

    int nsubs = 0;
    
    struct node *cur = subscribers->root;
    while (cur != NULL) {
        struct subscriber *sub = cur->entry;

        if (!subscriber_dead(sub)) {
            nsubs++;
        }
        
        cur = cur->next;
    }

    return nsubs;
}

int topic_add_message(struct list *topics, struct list *messages,
        char *topicname, char *content){

    int ret; // to check other methods return values
    int val = -1; // this return value
    struct topic *topic;

    // acquire topics list lock
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);

    topic = find_topic(topics, topicname);
    if (topic == NULL) {
        val = TOPIC_NOT_FOUND;
    } else {
        // acquire subscribers list readlock
        ret = pthread_rwlock_rdlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        if (count_alive_subscriber(topic->subscribers) == 0) {
            val = TOPIC_NO_SUBSCRIBERS;

            // release subscribers list readlock
            ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
            assert(ret == 0);
        } else {

            // create message and copy subscribers

            struct message *msg = malloc(sizeof(struct message));
            message_init(msg);
            msg->content = strdup(content);
            msg->topicname = strdup(topicname);

            // add statistics entry for each alive subscriber
            struct node *cur = topic->subscribers->root;
            while (cur != NULL) {
                struct subscriber *sub = cur->entry;

                if (!subscriber_dead(sub)) {
                    struct msg_statistics *stat =
                        malloc(sizeof(struct msg_statistics));
                    msg_statistics_init(stat);
                    stat->last_fail = 0;
                    stat->nattempts = 0;
                    stat->subscriber = sub;
                    int ret = list_add(msg->stats, stat);
                    assert(ret == 0);
                }

                cur = cur->next;
            }

            // release subscribers list readlock
            ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
            assert(ret == 0);

            // acquire write lock for message list
            ret = pthread_rwlock_wrlock(messages->listrwlock);
            assert(ret == 0);

            ret = list_add(messages, msg);
            assert(ret == 0);

            // release write lock for messages list
            ret = pthread_rwlock_unlock(messages->listrwlock);
            assert(ret == 0);

            val = 0;
        }
    }

    // release topics list readlock
    ret = pthread_rwlock_unlock(topics->listrwlock);
    assert(ret == 0);

    return val;
}

static int same_subscriber(const void *a, const void *b) {
    const struct msg_statistics *as = a;
    const struct msg_statistics *bs = b;
    return as->subscriber == bs->subscriber;
}

int message_remove_subscriber(struct list *messages,
        struct subscriber *subscriber) {
    int ret;

    // to compare against
    struct msg_statistics stat;
    stat.subscriber = subscriber;

    // acquire lock of message list
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *cur = messages->root;
    for (;cur != NULL; cur = cur->next) {
        struct message *msg = cur->entry;

        // acquire write lock of statistics list
        ret = pthread_rwlock_wrlock(msg->stats->listrwlock);
        assert(ret == 0);

        ret = list_remove_generic(msg->stats, same_subscriber, &stat);
        // not found is ok since we just try
        assert(ret == LIST_NOT_FOUND || ret == 0);

        // release write lock of statistics list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);
    }

    // release read lock of message list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return 0;
}

void topic_strerror(int errcode, char *buf) {
    switch (errcode) {
        case TOPIC_NOT_FOUND:
            sprintf(buf, "TOPIC_NOT_FOUND");
            break;
        case TOPIC_NO_SUBSCRIBERS:
            sprintf(buf, "TOPIC_NO_SUBSCRIBERS");
            break;
        default:
            sprintf(buf, "UNKNOWN_ERROR");
    }
}

int topic_init(struct topic *topic) {

    int ret;

    topic->subscribers = malloc(sizeof(struct list));
    assert(topic->subscribers != NULL);

    ret = list_init(topic->subscribers);
    assert(ret == 0);

    return 0;
}

int topic_destroy(struct topic *topic) {

    list_clean(topic->subscribers);
    list_destroy(topic->subscribers);

    free(topic->subscribers);
    topic->subscribers = NULL;

    free(topic->name);
    topic->name = NULL;
    
    return 0;
}

int message_init(struct message *message) {
    message->content = NULL;
    message->topicname = NULL;
    struct list *stats = malloc(sizeof(struct list));
    assert(stats != NULL);
    list_init(stats);
    message->stats = stats;
    return 0;
}

int message_destroy(struct message *message) {
    list_destroy(message->stats);
    free(message->stats);
    message->stats = NULL;
    free(message->content);
    message->content = NULL;
    free(message->topicname);
    message->topicname = NULL;
    return 0;
}

int msg_statistics_init(struct msg_statistics *stat) {
    
    int ret;

    pthread_rwlock_t *statrwlock = malloc(sizeof(pthread_rwlock_t));
    assert(statrwlock != NULL);
    ret = pthread_rwlock_init(statrwlock, NULL);
    assert(ret == 0);

    stat->statrwlock = statrwlock;
    stat->last_fail = 0;
    stat->nattempts = 0;

    return 0;
}

int msg_statistics_destroy(struct msg_statistics *stat) {

    int ret;

    ret = pthread_rwlock_destroy(stat->statrwlock);
    assert(ret == 0);
    free(stat->statrwlock);
    stat->statrwlock = NULL;

    return 0;
}
