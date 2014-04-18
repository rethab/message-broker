#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "topic.h"

int list_init(struct list *list) {

    int ret;

    pthread_rwlock_t *rwlock = malloc(sizeof(pthread_rwlock_t));
    assert(rwlock != NULL);
    ret = pthread_rwlock_init(rwlock, NULL);
    assert(ret == 0);
    list->listrwlock = rwlock;

    list->root = NULL;

    return 0;
}

int list_destroy(struct list *list) {

    int ret;
    
    assert(list->root == NULL);

    ret = pthread_rwlock_destroy(list->listrwlock);
    assert(ret == 0);
    free(list->listrwlock);

    list->root = NULL;

    return 0;
}

int list_add(struct list *list, void *entry) {
    if (list->root == NULL) {
        list->root = malloc(sizeof (struct node));
        assert(list->root != NULL);
        list->root->entry = entry;
        list->root->next = NULL;
    } else {
        struct node *cur = list->root;
        for (;cur->next != NULL; cur = cur->next);
        cur->next = malloc(sizeof(struct list));
        assert(cur->next != NULL);
        cur->next->entry = entry;
        cur->next->next = NULL;
    }
    return 0;
}

int list_empty(struct list *list) {
    return list->root == NULL;
}

/* removes an element from the list based on the
 * compare function. the third arg is passed
 * to the compare function as the second parameter
 */
static int list_remove_generic(struct list *list,
        int (*cmp)(const void *, const void *),
        const void *entry) {
    // empty list
    if (list->root == NULL) return LIST_NOT_FOUND;

    // replace root
    if (cmp(list->root->entry, entry)) {
        struct node *nxt = list->root->next;
        free(list->root);
        list->root = nxt;
        return 0;
    }
    struct node *prev = list->root;
    struct node *cur = list->root->next;
    while (cur != NULL) {
        if (cmp(cur->entry, entry)) {
            prev->next = cur->next;
            free(cur);
            return 0;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
    return LIST_NOT_FOUND;
}

static int list_cmp_same_ref(const void *a, const void *b) {
    return a == b;
}

int list_remove(struct list *list, void *entry) {
    return list_remove_generic(list, list_cmp_same_ref, entry);
}

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

static struct topic *create_new_topic(struct list *topics, char *name) {
    int ret;
    struct topic *topic = malloc(sizeof(struct topic));
    assert(topic != NULL);
    topic->name = strdup(name);
    topic->subscribers = malloc(sizeof(struct list));
    assert(topic->subscribers != NULL);
    list_init(topic->subscribers);
    ret = list_add(topics, topic);
    if (ret != 0) {
        free(topic->subscribers);
        free(topic->name);
        free(topic);
        return NULL;
    }
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

        val = list_add(topic->subscribers, subscriber);

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

        if (topic == NULL) {
             val = TOPIC_CREATION_FAILED;
        } else {

            // acquire subscribers list write lock
            ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
            assert(ret == 0);

            val = list_add(topic->subscribers, subscriber);

            // release subscribers list write lock
            ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
            assert(ret == 0);
        }

        // release topics list write lock
        ret = pthread_rwlock_unlock(topics->listrwlock);
        assert(ret == 0);
    }

    return val;
}

int topic_remove_subscriber(struct list *topics,
            struct subscriber *subscriber) {

    int ret; // return value from other functions
    int val = 0; // return value from this function

    // acquire read lock of topics
    ret = pthread_rwlock_rdlock(topics->listrwlock);
    assert(ret == 0);

    struct node *cur = topics->root;
    while (cur != NULL) {
        struct topic *topic = cur->entry;

        // acquire write lock of subscribers
        ret = pthread_rwlock_wrlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        val = list_remove(topic->subscribers, subscriber);

        // release write lock of subscribers
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);

        if (val == LIST_NOT_FOUND) {
            // ok
            val = 0; // reset it would otherwise be returned
        } else if (val < 0)  {
            // something went seriously wrong
            break; // exit while loop
        }

        cur = cur->next;
    }

    // release read lock of topics
    ret = pthread_rwlock_unlock(topics->listrwlock);
    assert(ret == 0);

    return val;
}

int topic_add_message(struct list *topics, struct list *messages,
        char *topicname, char *content){

    int ret; // to check other methods return values
    int val = 0; // this return value
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

        if (list_empty(topic->subscribers)) {
            val = TOPIC_NO_SUBSCRIBERS;
        }
        
        // release subscribers list readlock
        ret = pthread_rwlock_unlock(topic->subscribers->listrwlock);
        assert(ret == 0);
    }

    // topics exists with subscribers, proceed regularly
    if (val == 0) {
        // message
        struct message *msg = malloc(sizeof(struct message));
        assert(msg != NULL);
        msg->content = strdup(content);
        msg->topicname = strdup(topicname);
        msg->stats = malloc(sizeof(struct list));
        list_init(msg->stats);
        assert(msg->stats != NULL);

        // statistics, subs from topic
        struct node *cur = topic->subscribers->root;
        while (cur != NULL) {
            struct subscriber *sub = cur->entry;
            struct msg_statistics *stat =
                malloc(sizeof(struct msg_statistics));
            assert(stat != NULL);
            stat->last_fail = 0;
            stat->nattempts = 0;
            stat->subscriber = sub;
            int ret = list_add(msg->stats, stat);
            if (ret != 0) {
                free(stat);
                free(msg->stats);
                free(msg);
                return ret;
            }

            cur = cur->next;
        }

        // acquire write lock for message list
        ret = pthread_rwlock_wrlock(messages->listrwlock);
        assert(ret == 0);

        val = list_add(messages, msg);

        // release write lock for messages list
        ret = pthread_rwlock_unlock(messages->listrwlock);
        assert(ret == 0);
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
    int ret; // return value from other functions
    int val = 0; // return value from this function

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

        val = list_remove_generic(msg->stats, same_subscriber, &stat);

        // release write lock of statistics list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);

        if (val == LIST_NOT_FOUND) {
            // ok
            val = 0; // reset to avoid returning an error
        } else if (val < 0) {
            // something went seriously wrong
            break; // exit for loop
        }
    }

    // release read lock of message list
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    return val;
}

void topic_strerror(int errcode, char *buf) {
    switch (errcode) {
        case TOPIC_CREATION_FAILED:
            sprintf(buf, "TOPIC_CREATION_FAILED");
            break;
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

    return 0;
}
