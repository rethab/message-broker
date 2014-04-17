#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#include <stdlib.h>
#include <unistd.h>

#include "stomp.h"

#define SOCKET_TOO_MUCH    -2
#define SOCKET_INVALID     -3
#define SOCKET_CLIENT_GONE -4
#define SOCKET_NECROMANCE  -5

/* structure used to communicate with the
 * client. mutex is used to synchronize
 * access to the file descriptor. note that
 * this cannot be an rwlock since
 * that would allow multiple reads at the same
 * time. tcp is full duplex though, which means
 * we can write from one side and write at the
 * same time, but only one reader or writer
 * simultaneously */
struct client {
    /* guards sockfd reading*/
    pthread_mutex_t *mutex_r;

    /* guards sockfd writing */
    pthread_mutex_t *mutex_w;

    /* socket to client */
    int sockfd;

    /* guards the dead flag */
    pthread_rwlock_t *deadrwlock;

    /* indicates whether the connection
     * to the client is dead. this lock
     * must only be acquired when either
     * the read or write lock for the socket
     * itself is held. 0 means it is not
     * dead and 1 means it is dead. death
     * might have come unexpected (failed to write)
     * or expected (orderly disconnect). */
    int dead;
};

/* initializes the client struct */
int client_init(struct client *client);

/* destroys a client and frees resources */
void client_destroy(struct client *client);

/* reads a command from the socket. it reads until
 * it sees the null byte or the maximum buffer size
 * is reached */
int socket_read_command(struct client *client, struct stomp_command *cmd);

/* sends a command to the client */
int socket_send_command(struct client *client, struct stomp_command cmd);

#endif
