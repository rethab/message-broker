#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "socket.h"

#define BUFSIZE 1024

int socket_read_command(struct client *client, struct stomp_command *cmd) {
    int ret;

    int pos;
    char buf[BUFSIZE];
    char c;


    // accquire lock to read entire command
    ret = pthread_mutex_lock(client->mutex_r);
    assert(ret == 0);

    pos = 0;
    while (pos < BUFSIZE) {
        ret = read(client->sockfd, &c, 1); 

        // error
        if (ret != 1) {

            // release lock 
            ret = pthread_mutex_unlock(client->mutex_r);
            assert(ret == 0);

            fprintf(stderr, "read: %s\n", strerror(errno));
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
    int ret, resplen;
    char *resp;

    ret = create_command(cmd, &resp);
    if (ret != 0) {
        char buf[32];
        stomp_strerror(ret, buf);
        fprintf(stderr, "Error: %s\n", buf);
        return -1;
    }

    resplen = strlen(resp) + 1;

    // acquire lock
    ret = pthread_mutex_lock(client->mutex_w);
    assert(ret == 0);

    ret = write(client->sockfd, resp, resplen);

    // acquire lock
    ret = pthread_mutex_unlock(client->mutex_w);
    assert(ret == 0);

    free(resp);
    if (ret == -1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return SOCKET_CLIENT_GONE;
    }

    return 0;
}
