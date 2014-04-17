#ifndef TOPIC_HEADER
#define TOPIC_HEADER

#include "socket.h"

#define TOPIC_CREATION_FAILED -2
#define TOPIC_NOT_FOUND       -3
#define TOPIC_NO_SUBSCRIBERS  -4

/* client interested in messages of a topic */
struct subscriber {

    /* client information */
    struct client *client;

    /* name of the subscriber, used at login */
    char *name;
};

/* each message is associated with a topic */
struct topic {
    /* name of the topic */
    char *name;  

    /* list of subscribers to this topic.
     * the list is guarded by its own lock
     * and is to be held accoring to the
     * list lock laws (see the list struct
     * definition) */
    struct list *subscribers;
};

/* statistics for a message. this exists
 * per subscriber for each message
 */
struct msg_statistics {

    /* guards the the last_fail
     * as well as the nattempts field
     * of this structure. this lock
     * must only be acquired if the
     * parent lock (list of statistics
     * of the message) is held in read
     * mode */
    pthread_rwlock_t *statrwlock;

    /* the last time the message sending failed,
     * unix timestamp, 0 if never failed before.
     */
    long last_fail;

    /* number of attempts made to deliver message */
    int nattempts;

    /* receiver of the message */
    struct subscriber *subscriber;

};

/* message waiting for delivery. exists
 * once per message in a topic and holds
 * statistics for all subscribers.
 */
struct message {
    /* content to be sent */
    char *content;

    /* name of the topic it belongs to */
    char *topicname;

    /* guards the list of statistics. is
     * to be held in write mode if the list
     * itself is modified and in read mode
     * if an entry of the list is read or 
     * modified. this lock must only be
     * acquired if the parent lock (list
     * of messages) is held in read mode */
    pthread_rwlock_t *statsrwlock;

    /* statistics of this message, per subscriber */
    struct list *stats;
};

/* adds the subscriber to the topic. if the
 * topic does not exist, it is created
 */
int topic_add_subscriber(struct list *topics, char *name,
    struct subscriber *subscriber);

/* removes the subscriber from all topics */
int topic_remove_subscriber(struct list *topics, struct subscriber *subscriber);

/* adds the message to the list of messages and
 * copies the subscribers from the corresponding
 * topic. if the topic does not exist, the error
 * TOPIC_NOT_FOUND is returned (topic is created
 * with the first subscriber) */
int topic_add_message(struct list *topics, struct list *messages,
        char *topicname, char *content);

/* removes a subscriber from the message statistics.
 * this means that if a previous delivery failed,
 * it will not be attempted again. if it is the last/only
 * subscriber for that message, the message is removed. */
int message_remove_subscriber(struct list *messages,
    struct subscriber *subscriber);

/* converts a topic error code (TOPIC_) to a string.
 * the buffer should be 32 bytes */
void topic_strerror(int errcode, char *buf);


#define LIST_NOT_FOUND -2

/* root node of a linked list */
struct list {

    /* guards the list as a whole.
     * it is to be held in write
     * lock if the list is modified
     * (add, remove) and in read
     * mode if the contents are
     * (e.g. statistics updated).
     */
    pthread_rwlock_t *listrwlock;

    /* root node of the linked list */
    struct node *root;
};

/* node in a list. guarded by
 * listrwlock (in list struct) as
 * a whole and (potentially) by
 * locks in individual entries
 */
struct node {
    /* value of the node */
    void *entry;

    /* pointer to next node. null
     * if this node marks the end */
    struct node *next;   
};

/* initialize the list*/
int list_init(struct list *list);

/* frees the list. must be empty */
int list_destroy(struct list *list);

/* adds an entry to the end of the list */
int list_add(struct list *list, void *entry);

/* removes an element from the list */
int list_remove(struct list *list, void *entry);

/* checks whether the list is empty */
int list_empty(struct list *list);

#endif
