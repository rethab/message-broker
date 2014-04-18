#ifndef BROKER_HEADER
#define BROKER_HEADER

#include "topic.h"

/* global list of everything */
struct broker_context {

    /* global list of topics */
    struct list *topics;

    /* global list of messages */
    struct list *messages;
};

/* initializes a broker context */
int broker_context_init(struct broker_context *ctx);

/* destroys a broker context */
int broker_context_destroy(struct broker_context *ctx);

#endif
