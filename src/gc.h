#ifndef GC_HEADER
#define GC_HEADER

#include "distributor.h"
#include "broker.h"

/* time to wait for until another 
 * attempt to clean all messages and
 * statistics by the garbage
 * collector is made. */
#define GC_PASS_TIMEOUT 1

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
 * parameter from the messages list. returns
 * the number of statistics removed */
int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible);

/* removes all messages passed in the second
 * parameter from the messages list. returns
 * the number of messages removed */
int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible);

/* runs the garbage collection once. to
 * be invoked by 'gc_main_loop' continuously. */
int gc_run_gc(struct broker_context *ctx);

/* main loop that runs gc functions. accepts
 * param of type 'struct broker_context' */
void *gc_main_loop(void *arg);

#endif
