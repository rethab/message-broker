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
#define DEFAULT_PORT 55664

#define EXIT    1
#define NO_EXIT 0

void show_error(char *fun) {
    fprintf(stderr, "Function %s returned error: %s\n",
        fun, strerror(errno));
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

/* threads for all components */
static pthread_t handler_thread;
static pthread_t gc_thread;
static pthread_t distributor_thread;


int main(int argc, char** argv) {

    int port;
    if (argc != 2) {
        port = DEFAULT_PORT;
        printf("Usng default port %d\n", port);
    } else {
        port = atoi(argv[1]);
    }

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

    pthread_join(handler_thread, NULL);
    pthread_join(gc_thread, NULL);
    pthread_join(distributor_thread, NULL);
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
            show_error("start_handler/accept");
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

    int sockfd, ret;
    struct sockaddr_in srvaddr;
    struct handler_params *params =
        malloc(sizeof(struct handler_params));
    if (params == NULL) {
        show_error("handle_clients/socket");
        return -1;
    }

    // init socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { show_error("handle_clients/socket"); return -1; }

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = INADDR_ANY;
    srvaddr.sin_port = htons(port);

    ret = bind(sockfd, (struct sockaddr *) &srvaddr, sizeof(srvaddr));
    if (ret != 0) { show_error("handle_clients/bind"); return -1; }

    ret = listen(sockfd, MAXPENDING);
    if (ret != 0) { show_error("handle_clients/listen"); return -1; }

    params->ctx = ctx;
    params->sock = sockfd;

    ret = pthread_create(&handler_thread, NULL, &start_handler, params);
    if (ret != 0) {
        show_error("handle_clients/pthread_create");
        fprintf(stderr, "Failed to start client handler\n");
        return -1;
    } else {
        printf("Waiting for clients to connect on port %d\n", port);
        printf("Successfully started client handler\n");
        return 0;
    }
}

int start_gc(struct broker_context *ctx) {
    int ret;

    printf("Starting gc.. ");

    ret = pthread_create(&gc_thread, NULL, &gc_main_loop, ctx);
    if (ret != 0) {
        show_error("start_gc/pthread_create");
        fprintf(stderr, "Failed to start gc\n");
        return -1;
    } else {
        printf("success\n");
        return 0;
    }
}

int start_distributor(struct broker_context *ctx) {
    int ret;

    printf("Starting distributor.. ");

    ret = pthread_create(&distributor_thread, NULL,
        &distributor_main_loop, ctx);
    if (ret != 0) {
        show_error("start_distributor/pthread_create");
        fprintf(stderr, "Failed to start distributor\n");
        return -1;
    } else {
        printf("success\n");
        return 0;
    }
}
