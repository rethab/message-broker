#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#include "distributor.h"
#include "socket.h"
#include "stomp.h"

/* returns timestamp */
static long now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

void *distributor_main_loop(void *arg) {
    return 0;   
}

/* at least read lock must be held by calling function */
static int is_eligible(struct msg_statistics *stat) {
    // already successfully delivered
    if (stat->last_fail == 0 && stat->nattempts > 0) {
        return 0;
    }

    long ts = now();
    if ( ((ts - stat->last_fail) > REDELIVERY_TIMEOUT) &&
         (stat->nattempts < MAX_ATTEMPTS)) {
        return 1;
    } else {
        return 0;
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
        printf("Sent message to subscriber\n");

        stat->last_fail = 0;
        stat->nattempts++;
    }
}

void deliver_messages(struct list messages) {

    int ret;

    // acquire read lock for list of messages
    ret = pthread_rwlock_rdlock(messages.listrwlock);
    assert(ret == 0);

    struct node *curMsg = messages.root;
    while (curMsg != NULL) {
        struct message *msg = curMsg->entry;

        // acquire lock for message stat list
        ret = pthread_rwlock_rdlock(msg->stats->listrwlock);
        assert(ret == 0);

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
                // acquire read lock for statistic
                ret = pthread_rwlock_wrlock(stat->statrwlock);
                assert(ret == 0);

                if (is_eligible(stat)) {
                    deliver_message(msg, stat);    
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
    ret = pthread_rwlock_unlock(messages.listrwlock);
    assert(ret == 0);
}
