#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#include "distributor.h"
#include "socket.h"
#include "stomp.h"
#include "broker.h"

/* returns timestamp */
static long now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void *distributor_main_loop(void *arg) {
    struct broker_context *ctx = arg;

    while (1) {
        deliver_messages(ctx->messages);
        sleep(DISTRIBUTOR_SEND_TIMEOUT);
    }
}

/* at least read lock must be held by calling function */
int is_eligible(struct msg_statistics *stat) {
    
    int ret;
    long ts = now();

    // already successfully delivered
    if (stat->last_fail == 0 && stat->nattempts > 0) {
        return 0;
    } else if ( ((ts - stat->last_fail) <= REDELIVERY_TIMEOUT) ||
         (stat->nattempts >= MAX_ATTEMPTS)) {
        return 0;
    } else {

        int val;
        struct client *client = stat->subscriber->client; 

        // lock dead flag to check if
        ret = pthread_mutex_lock(client->deadmutex);
        assert(ret == 0);
        
        if (client->dead) {
            val = 0;
        } else {
            val = 1;
        }

        // lock dead flag to check if
        ret = pthread_mutex_unlock(client->deadmutex);
        assert(ret == 0);

        return val;
    }

}

/* write lock must be held by calling function */
static void deliver_message(struct message *msg,
        struct msg_statistics *stat) {

    int ret;

    struct stomp_header header;
    header.key = "destination";
    header.val = msg->topicname;
    struct stomp_command cmd;
    cmd.name = "MESSAGE";
    cmd.headers = &header;
    cmd.nheaders = 1;
    cmd.content = msg->content;

    ret = socket_send_command(stat->subscriber->client, cmd);

    if (ret != 0) {
        fprintf(stderr,
                "Failed to send message to subscriber: %d\n",
                ret);

        stat->last_fail = now();
        stat->nattempts++;
    } else {
        stat->last_fail = 0;
        stat->nattempts++;
    }
}

void deliver_messages(struct list *messages) {

    int nmsgs = 0; // number of delivered messages
    int ret;

    // acquire read lock for list of messages
    ret = pthread_rwlock_rdlock(messages->listrwlock);
    assert(ret == 0);

    struct node *curMsg = messages->root;
    while (curMsg != NULL) {
        struct message *msg = curMsg->entry;

        // acquire lock for message stat list
        ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
        assert(ret == 0);


        // walk through statistics of a message and check
        // with read lock whether it is eligible. If yes,
        // check again with write lock and deliver.
        struct node *curStat = msg->stats->root;
        while (curStat != NULL) {
            struct msg_statistics *stat = curStat->entry;

            // acquire read lock for statistic
            ret = pthread_rwlock_rdlock(stat->statrwlock);
            assert(ret == 0);

            int eligible = is_eligible(stat);
            
            // release read lock for statistic
            ret = pthread_rwlock_unlock(stat->statrwlock);
            assert(ret == 0);

            if (eligible) {
                // acquire write lock for statistic
                ret = pthread_rwlock_wrlock(stat->statrwlock);
                assert(ret == 0);

                // check again to make sure
                if (is_eligible(stat)) {
                    deliver_message(msg, stat); // requires write lock
                    nmsgs++;
                }
                
                // release read lock for statistic
                ret = pthread_rwlock_unlock(stat->statrwlock);
                assert(ret == 0);
            }

            curStat = curStat->next;
        }

        // release lock for message stat list
        ret = pthread_rwlock_unlock(msg->stats->listrwlock);
        assert(ret == 0);


        curMsg = curMsg->next;
    }

    // release read lock for list of messages
    ret = pthread_rwlock_unlock(messages->listrwlock);
    assert(ret == 0);

    if (nmsgs != 0)
        printf("Distributor: Delivered %d Messages\n", nmsgs);
}
