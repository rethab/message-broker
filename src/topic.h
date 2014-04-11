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

struct list {
    void *entry;
    struct list *next;   
};

/* initializes a new list */
int init_list(struct list *root);

/* adds an entry to the end of the list */
int add_to_list(struct list *list, void *entry);

/* adds the subscriber to the topic. if the
 * topic does not exist, it is created
 */
int add_subscriber_to_topic(struct list *topics, char *name,
    struct subscriber *subscriber);

/* removes the subscriber identified by the 
 * subscriberid from all topics
 */
int remove_subscriber(struct list *topics, int subscriberid);
