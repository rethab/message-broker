#ifndef WORKER_HEADER
#define WORKER_HEADER

#include "socket.h"
#include "stomp.h"

/* params passed to the worker for each client */
struct worker_params {
    /* the client to be handled */
    struct client *client;

    /* global list of topics */
    struct list *topics;

    /* global list of messages */
    struct list *messages;
};

/* send an error message to the client with the specified reason */
int send_error(struct client *client, char *reason);

/* send receipt to client */
int send_receipt(struct client *client);

/* send connected message to client */
int send_connected(struct worker_params params);

/* add message sent by client to according topic */
int process_send(struct worker_params params, struct stomp_command cmd);

/* adds client to topic */
int process_subscribe(struct worker_params params,
    struct stomp_command cmd, struct subscriber *sub);

/* removes client from messages and topics */
int process_disconnect(struct worker_params params,
    struct subscriber *sub);

/* main loop for client. reads commands */
void handle_client(struct worker_params params);

#endif
