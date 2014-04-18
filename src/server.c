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
#include <assert.h>
#include <pthread.h>

#include "stomp.h"
#include "topic.h"
#include "broker.h"

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

    // init shared structures
    struct list messages;
    list_init(&messages);
    struct list topics;
    list_init(&topics);

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

        pthread_mutex_t mutex_w = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t mutex_r = PTHREAD_MUTEX_INITIALIZER;
        struct client client;
        client.mutex_w = &mutex_w;
        client.mutex_r = &mutex_r;
        client.sockfd = cli;
        struct worker_params params = {&client, &topics, &messages };
        handle_client(params);
    }

    // TODO handle ctrl-c
    close(sock);

    exit(EXIT_SUCCESS);
}
