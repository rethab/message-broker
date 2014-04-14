#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#include <stdlib.h>
#include <unistd.h>

#include "stomp.h"

#define SOCKET_TOO_MUCH    -2
#define SOCKET_INVALID     -3
#define SOCKET_CLIENT_GONE -4

/* structure used to communicate with the
 * client. mutex is used to synchronize
 * access to the file descriptor */
struct client {
    /* guards sockfd reading*/
    pthread_mutex_t *mutex_r;

    /* guards sockfd writing */
    pthread_mutex_t *mutex_w;

    /* socket to client */
    int sockfd;
};

/* reads a command from the socket. it reads until
 * it sees the null byte or the maximum buffer size
 * is reached */
int socket_read_command(struct client client, struct stomp_command *cmd);

/* sends a command to the client */
int socket_send_command(struct client client, struct stomp_command cmd);

#endif
