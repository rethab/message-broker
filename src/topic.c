#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "topic.h"

int list_init(struct list *list) {
    list->root = NULL;
    return 0;
}

int list_add(struct list *list, void *entry) {
    if (list->root == NULL) {
        list->root = malloc(sizeof (struct node));
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

int list_remove(struct list *list, void *entry) {
    // empty list
    if (list->root == NULL) return LIST_NOT_FOUND;

    // replace root
    if (list->root->entry == entry) {
        struct node *nxt = list->root->next;
        free(list->root);
        list->root = nxt;
        return 0;
    }
    struct node *prev = list->root;
    struct node *cur = list->root->next;
    while (cur != NULL) {
        if (cur->entry == entry) {
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
    topic->name = strdup(name); // MALLOC
    topic->subscribers = malloc(sizeof(struct list));
    assert(topic->subscribers != NULL);
    topic->subscribers->root = NULL;
    ret = list_add(topics, topic);
    if (ret != 0) return NULL;
    return topic;
}

int add_subscriber_to_topic(struct list *topics, char *name,
                struct subscriber *subscriber) {
    struct topic *topic = find_topic(topics, name);
    if (topic == NULL) {
        topic = create_new_topic(topics, name);
        if (topic == NULL) return TOPIC_CREATION_FAILED;
    }
    return list_add(topic->subscribers, subscriber);
}

int remove_subscriber(struct list *topics, struct subscriber *subscriber) {
    int ret;
    struct node *cur = topics->root;
    while (cur != NULL) {
        struct topic *topic = cur->entry;
        ret = list_remove(topic->subscribers, subscriber);
        if (ret == LIST_NOT_FOUND); // ok
        else if (ret < 0) return ret;

        cur = cur->next;
    }
    return 0;
}

int add_message_to_topic(struct list *topics, struct list *messages,
        char *topicname, char *content){

    struct topic *topic = find_topic(topics, topicname);
    if (topic == NULL) return TOPIC_NOT_FOUND;
    if (topic->subscribers->root == NULL) return TOPIC_NO_SUBSCRIBERS;

    // message
    struct message *msg = malloc(sizeof(struct message));
    assert(msg != NULL);
    msg->content = strdup(content);
    msg->topicname = strdup(topicname);
    msg->stats = malloc(sizeof(struct list));
    assert(msg->stats != NULL);

    // statistics, subs from topic
    struct node *cur = topic->subscribers->root;
    while (cur != NULL) {
        struct subscriber *sub = cur->entry;
        struct msg_statistics *stat = malloc(sizeof(struct msg_statistics));
        assert(stat != NULL);
        stat->last_fail = 0;
        stat->nattempts = 0;
        stat->subscriber = sub;
        int ret = list_add(msg->stats, stat);
        if (ret != 0) return ret;

        cur = cur->next;
    }

    return list_add(messages, msg);
}
