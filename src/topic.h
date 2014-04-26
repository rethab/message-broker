#ifndef TOPIC_HEADER
#define TOPIC_HEADER

/* topic.h
 *
 * the message broker has two central lists:
 *  1. topics
 *  2. messages
 * both of them are fields in the broker_context
 * struct that is passed around from top-level.
 *
 * 1. topics
 * whenever a client subscribers to a topic, an
 * entry in the topics list is added if the
 * topic does not exist yet. if it exists, the
 * subscriber is added to the list of subscribers
 * in that topic.
 *
 * 2. messages
 * for each message that gets sent to a topic,
 * the list of subscribers to that topic is copied
 * and an entry is added to the list of messages
 * containing the name of the topic the message
 * was sent to, the message itself and the list
 * of subscribers. the list of subscribers is not
 * copied 1:1 though, as weed a little more
 * information for each subscriber (e.g. whether
 * is was already delivered to that subscriber,
 * etc). Therefore, the list of subscribers in
 * the message list is a list of msg_statistics,
 * which contains the additional information.
 *
 */

#include "list.h"
#include "socket.h"

/* returned if a message is added to
 * a topic that does not exist. note
 * that a topic is created with the 
 * first subscriber and not with the first
 * message */
#define TOPIC_NOT_FOUND       -3

/* no subscribers in topic and therefore
 * it is not possible to add a message.
 * this may also be returned if only
 * dead subscribers are in the topic */
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
     * unix timestamp, 0 if never failed before
     * or last delivery was successful.
     */
    long last_fail;

    /* number of attempts made to deliver message. if
     * at least one attempt was made and the last one
     * was successful, then the entire message is
     * to be viewed as successful and must not
     * redelivered
     */
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

    /* statistics of this message, per subscriber */
    struct list *stats;
};

/* initializes a topic */
int topic_init(struct topic *topic);

/* destroys a topic */
int topic_destroy(struct topic *topic);

/* initializes a message */
int message_init(struct message *message);

/* destroys a message */
int message_destroy(struct message *message);

/* initializes message statistics */
int msg_statistics_init(struct msg_statistics *stat);

/* destroys message statistics */
int msg_statistics_destroy(struct msg_statistics *stat);

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
#endif
