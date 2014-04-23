#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "socket.h"

#define BUFSIZE 1024

static void set_client_dead(struct client *client) {

    int ret;

    // acquire dead write lock
    ret = pthread_mutex_lock(client->deadmutex);
    assert(ret == 0);

    client->dead = 1;

    // release dead write lock
    ret = pthread_mutex_unlock(client->deadmutex);
    assert(ret == 0);
}

int socket_read_command(struct client *client,
        struct stomp_command *cmd) {

    int ret;

    int pos;
    char buf[BUFSIZE];
    char c;

    // accquire lock to read entire command
    ret = pthread_mutex_lock(client->mutex_r);
    assert(ret == 0);

    // acquire deadmutex 
    ret = pthread_mutex_lock(client->deadmutex);
    assert(ret == 0);

    int dead = client->dead;

    // release deadmutex
    ret = pthread_mutex_unlock(client->deadmutex);
    assert(ret == 0);

    if (dead) {

        // release lock to read from socket
        ret = pthread_mutex_unlock(client->mutex_r);
        assert(ret == 0);

        return SOCKET_NECROMANCE;
    }

    pos = 0;
    while (pos < BUFSIZE) {
        ret = read(client->sockfd, &c, 1); 

        // error
        if (ret < 1) {

            if (ret < 0) {
                fprintf(stderr, "read: %s\n", strerror(errno));
            }

            // early release lock 
            ret = pthread_mutex_unlock(client->mutex_r);
            assert(ret == 0);

            set_client_dead(client);

            return SOCKET_CLIENT_GONE;
        } 

        buf[pos++] = c;

        // end of command
        if (c == '\0') {
            break;
        }
    }

    // release lock
    ret = pthread_mutex_unlock(client->mutex_r);
    assert(ret == 0);

    if (pos == BUFSIZE && buf[pos-1] != '\0') {
        return SOCKET_TOO_MUCH;
    } else {
        ret = parse_command(buf, cmd);

        if (ret != 0) {
            char errbuf[32];
            stomp_strerror(ret, errbuf);
            fprintf(stderr, "parse_command: %s\n", errbuf);
            return -1;
        } else {

            return 0;
        }
    }
}

int socket_send_command(struct client *client, struct stomp_command cmd) {
    int ret, resplen, val;
    char *resp;

    ret = create_command(cmd, &resp);
    if (ret != 0) {
        char buf[32];
        stomp_strerror(ret, buf);
        fprintf(stderr, "Error: %s\n", buf);
        return -1;
    }

    resplen = strlen(resp) + 1;

    // accquire lock to write entire command
    ret = pthread_mutex_lock(client->mutex_w);
    assert(ret == 0);

    // acquire deadmutex 
    ret = pthread_mutex_lock(client->deadmutex);
    assert(ret == 0);

    int dead = client->dead;

    // release deadmutex
    ret = pthread_mutex_unlock(client->deadmutex);
    assert(ret == 0);

    if (dead) {

        // release lock to write to socket
        ret = pthread_mutex_unlock(client->mutex_w);
        assert(ret == 0);

        return SOCKET_NECROMANCE;
    }

    val = write(client->sockfd, resp, resplen);

    // acquire lock
    ret = pthread_mutex_unlock(client->mutex_w);
    assert(ret == 0);

    free(resp);
    if (val == -1) {
        fprintf(stderr, "Error: %s\n", strerror(val));

        set_client_dead(client);

        return SOCKET_CLIENT_GONE;
    }

    return 0;
}

int socket_terminate_client(struct client *client) {

    int ret;

    // acquire write lock on dead variable
    ret = pthread_mutex_lock(client->deadmutex);
    assert(ret == 0);

    client->dead = 1; // may already be the case though

    // release lock for dead flag
    ret = pthread_mutex_unlock(client->deadmutex);
    assert(ret == 0);

    // may fail if already closed
    close(client->sockfd);

    return 0;
}

int client_init(struct client *client) {

    int ret;

    pthread_mutexattr_t deadattr;
    ret = pthread_mutexattr_init(&deadattr);
    assert(ret == 0);
    ret = pthread_mutexattr_settype(&deadattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_t *deadmutex = malloc(sizeof(pthread_mutex_t));
    assert(deadmutex != NULL);
    ret = pthread_mutex_init(deadmutex, &deadattr);
    assert(ret == 0);
    
    pthread_mutex_t *mutex_r = malloc(sizeof(pthread_mutex_t));
    assert(mutex_r != NULL);
    ret = pthread_mutex_init(mutex_r, NULL);
    assert(ret == 0);

    pthread_mutexattr_t wmutattr;
    ret = pthread_mutexattr_init(&wmutattr);
    assert(ret == 0);
    ret = pthread_mutexattr_settype(&wmutattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_t *mutex_w = malloc(sizeof(pthread_mutex_t));
    assert(mutex_w != NULL);
    ret = pthread_mutex_init(mutex_w, &wmutattr);
    assert(ret == 0);

    client->dead = 0;
    client->deadmutex = deadmutex;
    client->mutex_r = mutex_r;
    client->mutex_w = mutex_w;

    return 0;
}

void client_destroy(struct client *client) {

    int ret;

    ret = pthread_mutex_destroy(client->mutex_r);
    assert(ret == 0);
    free(client->mutex_r);
    client->mutex_r = NULL;

    ret = pthread_mutex_destroy(client->mutex_w);
    assert(ret == 0);
    free(client->mutex_w);
    client->mutex_w = NULL;

    ret = pthread_mutex_destroy(client->deadmutex);
    assert(ret == 0);
    free(client->deadmutex);
    client->deadmutex = NULL;
}
