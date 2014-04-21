#ifndef BROKER_HEADER
#define BROKER_HEADER

#include "stomp.h"

/* global list of everything */
struct broker_context {

    /* global list of topics */
    struct list *topics;

    /* global list of messages */
    struct list *messages;
};

/* params passed to handler thread */
struct handler_params {
    /* global ctx */
    struct broker_context *ctx;
    /* socket to listen on for clients */
    int sock;
};

/* initializes a broker context */
int broker_context_init(struct broker_context *ctx);

/* destroys a broker context */
int broker_context_destroy(struct broker_context *ctx);

#define WORKER_CONTINUE 2
#define WORKER_STOP     3
#define WORKER_ERROR    4

/* send an error message to the client with the specified reason */
int send_error(struct client *client, char *reason);

/* send receipt to client */
int send_receipt(struct client *client);

/* send connected message to client */
int send_connected(struct client *client);

/* add message sent by client to according topic */
int process_send(struct broker_context *ctx,
                 struct client *client,
                 struct stomp_command cmd);

/* adds client to topic */
int process_subscribe(struct broker_context *ctx,
                      struct stomp_command cmd,
                      struct subscriber *sub);

/* removes client from messages and topics */
int process_disconnect(struct broker_context *ctx,
                       struct client *client,
                       struct subscriber *sub);

/* main loops that is continuously invoked
 * while a client is connected. it returns
 * one of the WORKER_* constants and depending
 * on that value should be invoked again
 */
int main_loop(struct broker_context *ctx,
              struct client *client,
              int *connected,
              struct subscriber *sub);

/* main loop for client. reads commands, accepts
 * parameter of type 'struct handler_params' */
void * handle_client(void *handler_thread_params);

#endif
