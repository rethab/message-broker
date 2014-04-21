#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "topic.h"
#include "broker.h"

void * handle_client(void *handler_thread_params) {
    struct handler_params *params = handler_thread_params;
    struct broker_context *ctx = params->ctx;
    int sockfd = params->sock;

    int ret;
    struct client client;
    struct subscriber sub;
    int connected = 0;

    client_init(&client);
    client.sockfd = sockfd;

    do {
        ret = main_loop(ctx, &client, &connected, &sub);
    } while (ret == WORKER_CONTINUE);

    socket_terminate_client(&client);

    client_destroy(&client);

    return 0;
}

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

int send_connected(struct client *client) {
    struct stomp_command respc;

    respc.name = "CONNECTED";
    respc.headers = NULL;
    respc.nheaders = 0;
    respc.content = NULL;

    return socket_send_command(client, respc);
}

int process_send(struct broker_context *ctx,
                 struct client *client,
                 struct stomp_command cmd) {
    int ret;

    struct list *topics = ctx->topics;
    struct list *messages = ctx->messages;

    char *topic = cmd.headers[0].val; // has only one header
    char *content = cmd.content;

    ret = topic_add_message(topics, messages, topic, content);
    if (ret != 0) {
        char errmsg[32];
        topic_strerror(ret, errmsg);
        fprintf(stderr,
            "Error from topic_add_message: %s (%d)\n", errmsg, ret);
        ret = send_error(client, "Failed to add message");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");

        return -1;
    } else {
        printf("Added message '%s' to topic '%s'\n", content, topic);
        return 0;
    }
}

int process_subscribe(struct broker_context *ctx,
                      struct stomp_command cmd,
                      struct subscriber *sub) {
    int ret;

    struct list *topics = ctx->topics;
    char *topic = cmd.headers[0].val; // has only one header

    ret = topic_add_subscriber(topics, topic, sub);
    assert(ret == 0);

    return 0;
}

int process_disconnect(struct broker_context *ctx,
                       struct client *client,
                       struct subscriber *sub) {
    int ret;

    struct list *topics = ctx->topics;
    struct list *messages = ctx->messages;

    // remove from topic
    ret = topic_remove_subscriber(topics, sub);
    assert(ret == 0);

    // remove from message
    ret = message_remove_subscriber(messages, sub);
    assert(ret == 0);

    printf("Removed subscriber '%s' from message\n", sub->name);

    ret = send_receipt(client);
    if (ret != 0) {
        ret = send_error(client, "Failed to send receipt");

        if (ret != 0) fprintf(stderr, "Failed to send error\n");
        
    }
    return 0;
}

int main_loop(struct broker_context *ctx,
              struct client *client,
              int *connected,
              struct subscriber *sub) {

    int ret;
    struct stomp_command cmd;

    ret = socket_read_command(client, &cmd);
    if (ret == SOCKET_CLIENT_GONE || ret == SOCKET_NECROMANCE) {
        printf("Cient has gone\n");
        return WORKER_ERROR;
    } else if (ret == SOCKET_TOO_MUCH) {
        printf("Client has sent too much. Closing Connection\n");
        return WORKER_ERROR;
    } else {
        assert(ret == 0);
    }

    if (!(*connected)) {
        if (strcmp("CONNECT", cmd.name) != 0) {
            ret = send_error(client, "Expected CONNECT");
            return WORKER_CONTINUE;
        } else {
            *connected = 1;
            sub->client = client;
            sub->name = cmd.headers[0].val; // has only one header
            ret = send_connected(client);
            return WORKER_CONTINUE;
        }
    } else {
        if (strcmp("SEND", cmd.name) == 0) {
            ret = process_send(ctx, client, cmd);
            return WORKER_CONTINUE;
        } else if (strcmp("SUBSCRIBE", cmd.name) == 0) {
            ret = process_subscribe(ctx, cmd, sub);
            return WORKER_CONTINUE;
        } else if (strcmp("DISCONNECT", cmd.name) == 0) {
            ret = process_disconnect(ctx, client, sub);
            return WORKER_STOP;
        } else {
            // impossible, socket read would have failed
            assert(0);
            return WORKER_STOP;
        }
    }
}

int broker_context_init(struct broker_context *ctx) {
    int ret;

    ret = list_init(ctx->messages);
    assert(ret == 0);

    ret = list_init(ctx->topics);
    assert(ret == 0);

    return 0;
}

int broker_context_destroy(struct broker_context *ctx) {
    int ret;

    ret = list_destroy(ctx->messages);
    assert(ret == 0);

    ret = list_destroy(ctx->topics);
    assert(ret == 0);

    return 0;
}
