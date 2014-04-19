#include "gc.h"

int gc_eligible_stat(struct msg_statistics *stat) {
    return -1;
}

int gc_eligible_msg(struct message *msg) {
    return -1;
}

int gc_collect_eligible_stats(struct list *messages,
                              struct list *eligible) {
    return -1;
}

int gc_collect_eligible_msgs(struct list *messages,
                             struct list *eligible) {
    return -1;
}

int gc_remove_eligible_stats(struct list *messages,
                             struct list *eligible) {
    return -1;
}

int gc_remove_eligible_msgs(struct list *messages,
                            struct list *eligible) {
    return -1;
}
