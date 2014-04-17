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
    ret = pthread_rwlock_wrlock(client->deadrwlock);
    assert(ret == 0);

    client->dead = 1;

    // release dead write lock
    ret = pthread_rwlock_unlock(client->deadrwlock);
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

    // acquire deadrwlock 
    ret = pthread_rwlock_rdlock(client->deadrwlock);
    assert(ret == 0);

    int dead = client->dead;

    // release deadrwlock
    ret = pthread_rwlock_unlock(client->deadrwlock);
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
        if (ret != 1) {

            // early release lock 
            ret = pthread_mutex_unlock(client->mutex_r);
            assert(ret == 0);

            fprintf(stderr, "read: %s\n", strerror(errno));

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

    // acquire deadrwlock 
    ret = pthread_rwlock_rdlock(client->deadrwlock);
    assert(ret == 0);

    int dead = client->dead;

    // release deadrwlock
    ret = pthread_rwlock_unlock(client->deadrwlock);
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

int client_init(struct client *client) {

    int ret;

    pthread_rwlock_t *deadrwlock = malloc(sizeof(pthread_rwlock_t));
    assert(deadrwlock != NULL);
    ret = pthread_rwlock_init(deadrwlock, NULL);
    assert(ret == 0);
    
    pthread_mutex_t *mutex_r = malloc(sizeof(pthread_mutex_t));
    assert(mutex_r != NULL);
    ret = pthread_mutex_init(mutex_r, NULL);
    assert(ret == 0);

    pthread_mutex_t *mutex_w = malloc(sizeof(pthread_mutex_t));
    assert(mutex_w != NULL);
    ret = pthread_mutex_init(mutex_w, NULL);
    assert(ret == 0);

    client->dead = 0;
    client->deadrwlock = deadrwlock;
    client->mutex_r = mutex_r;
    client->mutex_w = mutex_w;

    return 0;
}

void client_destroy(struct client *client) {

    int ret;

    ret = pthread_mutex_destroy(client->mutex_r);
    assert(ret == 0);
    free(client->mutex_r);

    ret = pthread_mutex_destroy(client->mutex_w);
    assert(ret == 0);
    free(client->mutex_w);

    ret = pthread_rwlock_destroy(client->deadrwlock);
    assert(ret == 0);
    free(client->deadrwlock);
}
