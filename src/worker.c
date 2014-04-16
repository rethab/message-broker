#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "topic.h"
#include "socket.h"
#include "worker.h"

int send_error(struct client *client, char *reason) {
    struct stomp_command respc;
    struct stomp_header header;

    respc.name = "ERROR";
    header.key = "message";
    header.val = reason;
    respc.headers = &header;
    respc.nheaders = 1;
    respc.content = NULL;

    return socket_send_command(client, respc);
}

int send_receipt(struct client *client) {
    struct stomp_command respc;

    respc.name = "RECEIPT";
    respc.headers = NULL;
    respc.nheaders = 0;
    respc.content = NULL;

    return socket_send_command(client, respc);
}

int send_connected(struct worker_params params) {
    struct stomp_command respc;

    respc.name = "CONNECTED";
    respc.headers = NULL;
    respc.nheaders = 0;
    respc.content = NULL;

    return socket_send_command(params.client, respc);
}

int process_send(struct worker_params params, struct stomp_command cmd) {
    int ret;

    struct list *topics = params.topics;
    struct list *messages = params.messages;
    char *topic = cmd.headers[0].val; // has only one header
    char *content = cmd.content;

    ret = topic_add_message(topics, messages, topic, content);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_add_message: %d\n", ret);
        ret = send_error(params.client, "Failed to add message");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");

        return -1;
    } else {
        printf("Added message '%s' to topic '%s'\n", content, topic);
        return 0;
    }
}

int process_subscribe(struct worker_params params,
        struct stomp_command cmd, struct subscriber *sub) {
    int ret;

    struct list *topics = params.topics;
    char *topic = cmd.headers[0].val; // has only one header

    ret = topic_add_subscriber(topics, topic, sub);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_add_subscriber: %d\n", ret);
        ret = send_error(params.client, "Failed to add subscriber");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");

        return -1;
    } else {
        printf("Added subscriber '%s' to topic '%s'\n", sub->name, topic);
        return 0;
    }
}

int process_disconnect(struct worker_params params,
        struct subscriber *sub) {
    int ret;

    struct list *topics = params.topics;
    struct list *messages = params.messages;

    // remove from topic
    ret = topic_remove_subscriber(topics, sub);
    if (ret != 0) {
        fprintf(stderr, "Error from topic_remove_subscriber: %d\n", ret);
        ret = send_error(params.client,
            "Failed to remove subscriber from topic");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");
        return ret;

    } else {
        // remove from message
        ret = message_remove_subscriber(messages, sub);
        if (ret != 0) {
            fprintf(stderr, "Error from message_remove_subscriber: %d\n", ret);
            ret = send_error(params.client, "Failed to add subscriber");

            if (ret != 0) fprintf(stderr, "Failed to send error\n");

            return -1;
        } else {
            printf("Removed subscriber '%s' from message\n", sub->name);

            ret = send_receipt(params.client);
            if (ret != 0) {
                ret = send_error(params.client, "Failed to send receipt");

                if (ret != 0) fprintf(stderr, "Failed to send error\n");
                
            }
            return 0;
        }
    }
}

void handle_client(struct worker_params params) {
    int ret;
    int connected;
    struct stomp_command cmd;
    struct subscriber sub;

    connected = 0;
    while (1) {
        ret = socket_read_command(params.client, &cmd);
        if (ret != 0) {
            ret = send_error(params.client, "Failed to parse");
            continue;
        }

        if (!connected) {
            if (strcmp("CONNECT", cmd.name) != 0) {
                ret = send_error(params.client, "Expected CONNECT");
                continue;
            } else {
                connected = 1;
                sub.client = params.client;
                sub.name = cmd.headers[0].val; // has only one header
                ret = send_connected(params);
                continue;
            }
        } else {
            if (strcmp("SEND", cmd.name) == 0) {
                ret = process_send(params, cmd);
            } else if (strcmp("SUBSCRIBE", cmd.name) == 0) {
                ret = process_subscribe(params, cmd, &sub);
            } else if (strcmp("DISCONNECT", cmd.name) == 0) {
                ret = process_disconnect(params, &sub);
                return;
            } else {
                ret = send_error(params.client, "Unexpected command");
                fprintf(stderr, "Unexpected Command %s\n", cmd.name);
            }
                
        }

    }
}
