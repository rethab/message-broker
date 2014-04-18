#ifndef GC_HEADER
#define GC_HEADER

#include "distributor.h"

/* checks whether a statistics is eligible to
 * be garbage collected */
int gc_eligible_stat(struct msg_statistics *stat);

/* checks whether a message is eligible to
 * be garbage collected */
int gc_eligible_msg(struct message *msg);

/* collects all statistics to be garbage collected
 * in the second parameter */
int gc_collect_eligible_stats(struct list *messages,
                              struct list *eligible);

/* collects all messages to be garbage collected
 * in the second parameter */
int gc_collect_eligible_msgs(struct list *messages,
                             struct list *eligible);

/* removes all statstics passed in the second
 * parameter from the messages list */
int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible);

/* removes all messages passed in the second
 * parameter from the messages list */
int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible);

#endif
