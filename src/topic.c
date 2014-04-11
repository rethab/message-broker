#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "topic.h"

int init_list(struct list *list) {
    list->entry = NULL;
    list->next = NULL;
    return 0;
}

int add_to_list(struct list *list, void *entry) {
    struct list *cur = list;
    for (;cur->next != NULL; cur = cur->next);
    cur->next = malloc(sizeof(struct list));
    assert(cur->next != NULL);
    cur->next->entry = entry;
    cur->next->next = NULL;
    return 0;
}

int add_subscriber_to_topic(struct list *topics, char *name,
                struct subscriber *subscriber) {

    // first entry
    if (topics->entry == NULL) {
        struct list *subscribers = malloc(sizeof(struct list)); // MALLOC
        assert(subscribers != NULL);
        subscribers->entry = subscriber;
        subscribers->next = NULL;

        struct topic *topic = malloc(sizeof(struct topic)); // MALLOC
        assert(topic != NULL);
        topic->name = strdup(name); // MALLOC
        topic->subscribers = subscribers;

        topics->entry = topic;
        return 0;
    }

    // find existing
    struct list *cur = topics;
    do {
        struct topic *topic = cur->entry;
        if (strcmp(topic->name, name) == 0) {
            // append subscriber
            return add_to_list(topic->subscribers, subscriber);
        }

        if (cur->next == NULL) break;
        else cur = cur->next;

    } while (1);

    // create topic
    struct list *subscribers = malloc(sizeof(struct list)); // MALLOC
    assert(subscribers != NULL);
    subscribers->entry = subscriber;
    subscribers->next = NULL;

    struct topic *topic = malloc(sizeof(struct topic)); // MALLOC
    assert(topic != NULL);
    topic->name = strdup(name); // MALLOC
    topic->subscribers = subscribers;

    cur->next = malloc(sizeof(struct list));
    assert(cur->next != NULL);
    cur->next->entry = topic;
    cur->next->next = NULL;
    return 0;
       
}

int remove_subscriber(struct list *topics, int subscriberid) {
    
}
