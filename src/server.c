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
#include <pthread.h>

#include "topic.h"
#include "broker.h"
#include "gc.h"
#include "distributor.h"

#define MAXPENDING 10
#define BUFSIZE    1024

#define EXIT    1
#define NO_EXIT 0

void show_error() {
    fprintf(stderr, "Error: %s\n", strerror(errno));
}

/* starts the socket listener, waits for
 * incoming clients and accepts commands
 * from them.
 */
int handle_clients(int port, struct broker_context *ctx);

/* starts the garbage collecting thread */
int start_gc(struct broker_context *ctx);

/* starts the distributor thread */
int start_distributor(struct broker_context *ctx);


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    struct broker_context ctx;
    broker_context_init(&ctx);

    if (handle_clients(port, &ctx) == 0 &&
        start_gc(&ctx) == 0 &&
        start_distributor(&ctx) == 0) {
        printf("All components started successfully\n");
    } else {
        printf("Aborting..\n");
        broker_context_destroy(&ctx);
        exit(EXIT_FAILURE);
    }
}

static void * start_handler(void *arg) {
    int cli;
    struct sockaddr_in clientaddr;
    socklen_t clilen = sizeof(clientaddr);
    struct handler_params *params = arg;

    while (1) {
        cli = accept(params->sock,
                     (struct sockaddr *) &clientaddr,
                     &clilen);
        if (cli == -1) {
            show_error();
            continue;
        }

        struct handler_params client_handler_params;
        client_handler_params.ctx = params->ctx;
        client_handler_params.sock = cli;

        pthread_t handler_thread;
        pthread_create(&handler_thread, NULL,
            &handle_client, &client_handler_params);
    }

    return 0;
}

int handle_clients(int port, struct broker_context *ctx) {
    printf("Starting client handler..\n");

    int sock, ret;
    struct sockaddr_in srvaddr;
    pthread_t thread;
    struct handler_params params;

    // init socket
    sock  = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) { show_error(); return -1; }

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(port);

    ret = bind(sock, (struct sockaddr *) &srvaddr, sizeof(srvaddr));
    if (ret != 0) { show_error(); return -1; }

    ret = listen(sock, MAXPENDING);
    if (ret != 0) { show_error(); return -1; }

    params.ctx = ctx;
    params.sock = sock;

    pthread_create(&thread, NULL, &start_handler, &params);
    if (ret != 0) { show_error(); return -1; }

    printf("Waiting for clients to connect on port %d\n", port);

    return 0;
}

int start_gc(struct broker_context *ctx) {
    pthread_t thread;
    return pthread_create(&thread, NULL, &gc_main_loop, ctx);
}

int start_distributor(struct broker_context *ctx) {
    pthread_t thread;
    return pthread_create(&thread, NULL, &distributor_main_loop, ctx);
}
