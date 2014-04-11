#define TOPIC_CREATION_FAILED -2

/* client interested in messages of a topic */
struct subscriber {

    /* unique internal id */
    int id;

    /* file desriptor used for communication */
    int sockfd;

    /* name of the subscriber, used at login */
    char *name;
};

/* each message is associated with a topic */
struct topic {
    /* name of the topic */
    char *name;  

    /* list of subscribers to this topic */
    struct list *subscribers;
};

/* statistics for a message. this exists
 * per subscriber for each message
 */
struct msg_statistics {
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

    /* pointer to the topic it belongs to */
    struct topic *topic;

    /* statistics of this message, per subscriber */
    struct list *stats;
};

/* adds the subscriber to the topic. if the
 * topic does not exist, it is created
 */
int add_subscriber_to_topic(struct list *topics, char *name,
    struct subscriber *subscriber);

/* removes the subscriber from all topics */
int remove_subscriber(struct list *topics, struct subscriber *subscriber);

#define LIST_NOT_FOUND -2

struct list {
    struct node *root;
};

struct node {
    void *entry;
    struct node *next;   
};

/* initialize the list*/
int list_init(struct list *list);

/* adds an entry to the end of the list */
int list_add(struct list *list, void *entry);

/* removes an element from the list */
int list_remove(struct list *list, void *entry);
