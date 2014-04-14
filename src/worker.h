#ifndef WORKER_HEADER
#define WORKER_HEADER

#include "stomp.h"

/* params passed to the worker for each client */
struct worker_params {
    /* the client to be handled */
    int sockfd;

    /* global list of topics */
    struct list *topics;

    /* global list of messages */
    struct list *messages;
};

/* send an error message to the client with the specified reason */
int send_error(int clientfd, char *reason);

/* send receipt to client */
int send_receipt(struct worker_params params);

/* send connected message to client */
int send_connected(struct worker_params params);

/* process command send */
int process_send(struct worker_params params, struct stomp_command cmd);

/* process command subscribe */
int process_subscribe(struct worker_params params,
    struct stomp_command cmd, struct subscriber *sub);

/* process command disconnect */
int process_disconnect(struct worker_params params,
    struct subscriber *sub);

/* main loop for client. reads commands */
void handle_client(struct worker_params params);

#endif
