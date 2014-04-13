#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "stomp.h"

#define MAXPENDING 10
#define BUFSIZE    1024

#define EXIT    1
#define NO_EXIT 0

void show_error(int mode) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    if (mode == EXIT) exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int sock, cli, ret;
    struct sockaddr_in srvaddr, clientaddr;
    socklen_t clilen = sizeof(clientaddr);

    // init socket
    sock  = socket(PF_INET, SOCK_STREAM, 0);
    if (sock <= 0) show_error(EXIT);
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(port);
    ret = bind(sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr));
    if (ret != 0) show_error(EXIT);
    ret = listen(sock, MAXPENDING);
    if (ret != 0) show_error(EXIT);
    printf("Waitng for clients to connect on port %d\n", port);

    while (1) {
        cli = accept(sock, (struct sockaddr *) &clientaddr, &clilen);
        if (cli == -1) {
            show_error(NO_EXIT);
            continue;
        }

        char buf[BUFSIZE];
        printf("Before reading from client\n");
        size_t nbytes = read(cli, &buf, BUFSIZE-1);
        if (nbytes == -1) {
            show_error(NO_EXIT);
            continue;
        }
        buf[nbytes] = '\0';
        printf("After reading %zu bytes from client: '%s'\n", nbytes, buf);

        char *resp;
        struct stomp_command respc;
        struct stomp_header header;

        struct stomp_command cmd;
        ret = parse_command(buf, &cmd);
        if (ret != 0) {
            printf("About to send ERROR: %d\n", ret);
            respc.name = "ERROR";
            header.key = "message";
            header.val = "Failed to parse";
            respc.headers = &header;
            respc.nheaders = 1;
            respc.content = NULL;
            ret = create_command(respc, &resp);
            if (ret == -1) {
                fprintf(stderr, "Internal Error: %d\n", ret);
                continue;
            }
            ret = write(cli, resp, strlen(resp));
            if (ret == -1) {
                show_error(NO_EXIT);
                continue;
            } else {
                printf("Replied to Client: '%s'\n", resp);
            }
            continue;
        }

        if (strcmp("CONNECT", cmd.name) != 0) {
            printf("About to send ERROR\n");
            respc.name = "ERROR";
            header.key = "message";
            header.val = "Expected CONNECT";
            respc.headers = &header;
            respc.nheaders = 1;
            respc.content = NULL;
            ret = create_command(respc, &resp);
            if (ret == -1) {
                fprintf(stderr, "Internal Error: %d\n", ret);
                continue;
            }
            ret = write(cli, resp, strlen(resp));
            if (ret == -1) {
                show_error(NO_EXIT);
                continue;
            } else {
                printf("Replied to Client: '%s'\n", resp);
            }
        } else {
            printf("About to send SUCCESS\n");
            respc.name = "CONNECTED";
            respc.headers = NULL;
            respc.nheaders = 0;
            respc.content = NULL;
            ret = create_command(respc, &resp);
            if (ret == -1) {
                fprintf(stderr, "Internal Error: %d\n", ret);
                continue;
            }
            ret = write(cli, resp, strlen(resp));
            if (ret == -1) {
                show_error(NO_EXIT);
                continue;
            } else {
                printf("Replied to Client: '%s'\n", resp);
            }

        } 

    }

    // TODO handle ctrl-c
    close(sock);

    exit(EXIT_SUCCESS);
}
