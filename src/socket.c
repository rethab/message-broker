#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "socket.h"

#define BUFSIZE 1024

int socket_read_command(struct client client, struct stomp_command *cmd) {
    int ret;

    int pos;
    char buf[BUFSIZE];
    char c;

    pos = 0;
    while (pos < BUFSIZE) {
        ret = read(client.sockfd, &c, 1); 
        if (ret != 1) {
            return ret;
            break;
        }

        buf[pos++] = c;

        // end of command
        if (c == '\0') {
            break;
        }
    }

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

int socket_send_command(struct client client, struct stomp_command cmd) {
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
    ret = write(client.sockfd, resp, resplen);
    free(resp);
    if (ret == -1) {
        fprintf(stderr, "Error: %d: %s\n", errno, strerror(errno));
        return SOCKET_CLIENT_GONE;
    }

    return 0;
}
