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
#include "topic.h"

#define MAXPENDING 10
#define BUFSIZE    1024

#define EXIT    1
#define NO_EXIT 0

// TODO possibly move to worker.c
struct worker_params {
    /* the client to be handled */
    int sockfd;

    /* global list of topics */
    struct list *topics;

    /* global list of messages */
    struct list *messages;
};

void show_error(int mode) {
    fprintf(stderr, "Error: %s\n", strerror(errno));
    if (mode == EXIT) exit(EXIT_FAILURE);
}

int read_command(int sock, struct stomp_command *cmd) {
    printf("ENTERING: read_command\n");

    int ret;

    int pos;
    char buf[BUFSIZE];
    char c;

    pos = 0;
    while (pos < BUFSIZE) {
        if (read(sock, &c, 1) != 1) {
            show_error(NO_EXIT);
            break;
        }

        buf[pos++] = c;

        // end of command
        if (c == '\0') {
            break;
        }
    }

    if (pos == BUFSIZE && buf[pos-1] != '\0') {
        fprintf(stderr, "too much input\n");
        return -1;
    } else {
        ret = parse_command(buf, cmd);

        if (ret != 0)
            fprintf(stderr, "Failed to parse: '%s': %d\n", buf, ret);

        printf("EXITING: read_command\n");
        return ret;
    }
}

int send_command(int sock, struct stomp_command cmd) {
    printf("ENTERING: send_command\n");

    int ret, resplen;
    char *resp;

    ret = create_command(cmd, &resp);
    if (ret != 0) {
        fprintf(stderr, "Internal Error: %d\n", ret);
        return -1;
    }

    resplen = strlen(resp);
    ret = write(sock, resp, resplen);
    free(resp);
    if (ret == -1) {
        show_error(NO_EXIT);
        return -1;
    }

    printf("EXITING: send_command\n");

    return 0;
}

int send_error(int sock, char *reason) {
    printf("ENTERING: send_error\n");

    int ret;
    struct stomp_command respc;
    struct stomp_header header;

    respc.name = "ERROR";
    header.key = "message";
    header.val = reason;
    respc.headers = &header;
    respc.nheaders = 1;
    respc.content = NULL;

    ret = send_command(sock, respc);

    printf("EXITING: send_error\n");

    return ret;
}

int send_receipt(int sock) {
    printf("ENTERING: send_receipt\n");

    int ret;
    struct stomp_command respc;

    respc.name = "RECEIPT";
    respc.headers = NULL;
    respc.nheaders = 0;
    respc.content = NULL;

    ret = send_command(sock, respc);

    printf("EXITING: send_receipt\n");

    return ret;
}

int send_connected(int sock) {
    printf("ENTERING: send_connected\n");

    int ret; 
    struct stomp_command respc;

    respc.name = "CONNECTED";
    respc.headers = NULL;
    respc.nheaders = 0;
    respc.content = NULL;

    ret = send_command(sock, respc);

    printf("EXITING: send_connected\n");
    return ret;
}

int process_send(struct stomp_command cmd, struct worker_params params) {
    printf("ENTERING: process_send\n");
    int ret;

    struct list *topics = params.topics;
    struct list *messages = params.messages;
    char *topic = cmd.headers[0].val; // has only one header
    char *content = cmd.content;

    ret = topic_add_message(topics, messages, topic, content);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_add_message: %d\n", ret);
        ret = send_error(params.sockfd, "Failed to add message");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");

        return -1;
    } else {
        printf("Added message '%s' to topic '%s'\n", content, topic);
        return 0;
    }
    printf("EXITING: process_send\n");
}

int process_subscribe(struct stomp_command cmd,
        struct worker_params params, struct subscriber *sub) {
    printf("ENTERING: process_subscribe\n");
    int ret;

    struct list *topics = params.topics;
    char *topic = cmd.headers[0].val; // has only one header

    ret = topic_add_subscriber(topics, topic, sub);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_add_subscriber: %d\n", ret);
        ret = send_error(params.sockfd, "Failed to add subscriber");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");

        return -1;
    } else {
        printf("Added subscriber '%s' to topic '%s'\n", sub->name, topic);
        return 0;
    }
    printf("EXITING: process_subscribe\n");
}

int process_disconnect(struct worker_params params,
        struct subscriber *sub) {
    printf("ENTERING: process_disconnect\n");
    int ret;

    struct list *topics = params.topics;
    struct list *messages = params.messages;

    // remove from topic
    ret = topic_remove_subscriber(topics, sub);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_remove_subscriber: %d\n", ret);
        ret = send_error(params.sockfd,
            "Failed to remove subscriber from topic");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");
        return ret;

    } else {
        // remove from message
        ret = message_remove_subscriber(messages, sub);
        if (ret != 0) {
            fprintf(stderr, "Error from message_remove_subscriber: %d\n", ret);
            ret = send_error(params.sockfd, "Failed to add subscriber");

            if (ret != 0) fprintf(stderr, "Failed to send error\n");

            return -1;
        } else {
            printf("Removed subscriber '%s' from message\n", sub->name);

            ret = send_receipt(params.sockfd);
            if (ret != 0) {
                ret = send_error(params.sockfd, "Failed to send receipt");

                if (ret != 0) fprintf(stderr, "Failed to send error\n");
                
            }
            return 0;
        }
    }


    printf("EXITING: process_disconnect\n");
}

void handle_client(struct worker_params params) {
    int ret;
    int connected;
    struct stomp_command cmd;
    struct subscriber sub;

    connected = 0;
    while (1) {
        ret = read_command(params.sockfd, &cmd);
        if (ret != 0) {
            ret = send_error(params.sockfd, "Failed to parse");
            continue;
        }

        if (!connected) {
            if (strcmp("CONNECT", cmd.name) != 0) {
                ret = send_error(params.sockfd, "Expected CONNECT");
                continue;
            } else {
                connected = 1;
                sub.sockfd = params.sockfd;
                sub.name = cmd.headers[0].val; // has only one header
                ret = send_connected(params.sockfd);
                continue;
            }
        } else {
            if (strcmp("SEND", cmd.name) == 0) {
                ret = process_send(cmd, params);
            } else if (strcmp("SUBSCRIBE", cmd.name) == 0) {
                ret = process_subscribe(cmd, params, &sub);
            } else if (strcmp("DISCONNECT", cmd.name) == 0) {
                ret = process_disconnect(params, &sub);
            } else {
                ret = send_error(params.sockfd, "Unexpected command");
                fprintf(stderr, "Unexpected Command %s\n", cmd.name);
            }
                
        }

    }


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

        struct worker_params params = {cli, &topics, &messages };
        handle_client(params);
    }

    // TODO handle ctrl-c
    close(sock);

    exit(EXIT_SUCCESS);
}
