#ifndef DISTRIBUTOR_HEADER
#define DISTRIBUTOR_HEADER

/* the distributor periodically scans
 * the messages and tries to deliver them
 * to the respective subscribers
 */

#include "topic.h"

/* maximum number of attempts made to deliver
 * a message before the message may be removed
 */
#define MAX_ATTEMPTS 10 

/* timeout to wait between two attempts to deliver
 * the same message to a client that previously
 * rejected it
 */
#define REDELIVERY_TIMEOUT 2

/* searches the list of messages for messages 
 * to be sent to a subscriber. this may be for the first
 * time or because it is eligible to be resent.
 */
void deliver_messages(struct list *messages);

/* main loop that runs distributor functions. accepts
 * param of type 'struct broker_context' */
void *distributor_main_loop(void *arg);
 
#endif
