#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <pthread.h>

#define DEFAULT_PORT 55664

void error(char *msg) {
    perror(msg);
    exit(0);
}

struct thread_params {
    /* server to connect to */
    struct hostent *server;
    int port;

    char *name;
    int nmsgs;
};

/* connects to the server and returns
 * the socket file descriptor */
int connect_to_server(struct hostent *server, int port);

int send_connect(int sockfd, char *name);
int expect_connected(int sockfd);
int send_subscribe(int sockfd, char *dest);
int send_send(int sockfd, char *topic, char *content);
int send_disconnect(int sockfd, char *name);
int read_from_server(int sockfd, char *buf, size_t buflen);

void *subscriber(void *arg) {
    struct thread_params *params = arg;
    int sockfd = connect_to_server(params->server, params->port);
    char buf[1024];
    int ret;

    char myname[128];
    sprintf(myname, "subscriber/%s", params->name);

    ret = send_connect(sockfd, myname);
    if (ret != 0)
        fprintf(stderr, "subscriber: Failed to connect\n");
    ret = expect_connected(sockfd);
    if (ret != 0)
        fprintf(stderr, "subscriber: Missing connected\n");
    ret = send_subscribe(sockfd, "stocks");
    if (ret != 0)
        fprintf(stderr, "subscriber: Failed to subscribe to stocks\n");
    ret = send_subscribe(sockfd, "news");
    if (ret != 0)
        fprintf(stderr, "subscriber: Failed to subscribe to news\n");

    while (1) {
        ret = read_from_server(sockfd, buf, 1024);
        if (ret != 1) break;
        else fprintf(stderr, "subscriber: received: [%s]\n", buf);
    }

    return 0;
}

void *sender(void *arg) {
    int ret;
    struct thread_params *params = arg;
    int sockfd = connect_to_server(params->server, params->port);

    char myname[128];
    sprintf(myname, "sender/%s", params->name);

    ret = send_connect(sockfd, myname);
    if (ret != 0)
        fprintf(stderr, "sender: Failed to connect\n");
    ret = expect_connected(sockfd);
    if (ret != 0)
        fprintf(stderr, "sender: Missing connected\n");

    for (int i = 0; i < params->nmsgs; i++) {
        char msg[64];
        char *topic = i % 2 == 0 ? "news" : "stocks";
        sprintf(msg, "price: 22.%d", i);
        ret = send_send(sockfd, topic, msg);
        if (ret != 0)
            fprintf(stderr, "sender: Failed to send to stocks\n");
    }
    ret = send_disconnect(sockfd, "sender");
    if (ret != 0)
        fprintf(stderr, "sender: Failed to send disconnect\n");

    return 0;
}

int main(int argc, char *argv[]) {
    int ret;

    char *host;
    struct thread_params *params = malloc(sizeof(struct thread_params));

    if (argc == 1) {
        host = strdup("localhost");
        params->port = DEFAULT_PORT;
        params->name = strdup("ritschy");
        params->nmsgs = 42;
        fprintf(stderr, "Using default values. Host %s:%d\n", host, params->port);
    } else if (argc == 5) {
        host = argv[1];
        params->port = atoi(argv[2]); 
        params->name = strdup(argv[3]);
        params->nmsgs = atoi(argv[4]);
    } else {
        fprintf(stderr,"usage %s hostname port name nmsgs\n", argv[0]);
        fprintf(stderr,"\thostname: name of server\n");
        fprintf(stderr,"\tport: port of server\n");
        fprintf(stderr,"\tname: name to be used for login\n");
        fprintf(stderr,"\tnmsgs: number of messages to send\n");
        exit(0);
    }


    params->server = gethostbyname(host);

    if (params->server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    pthread_t t1, t2;
    ret = pthread_create(&t1, NULL, &subscriber, params);
    if (ret != 0) fprintf(stderr,"ERROR: %s\n", strerror(errno));

    sleep(1);

    pthread_create(&t2, NULL, &sender, params);
    if (ret != 0) fprintf(stderr,"ERROR: %s\n", strerror(errno));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    return 0;
}

int send_connect(int sockfd, char *name) {
    fprintf(stderr, "Send connect '%s'\n", name);
    char buf[64];
    sprintf(buf, "CONNECT\nlogin:%s\n\n", name);
    int n = write(sockfd, buf, strlen(buf) + 1);
    if (n != strlen(buf) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    } 
    return 0;
}

int send_subscribe(int sockfd, char *dest) {
    fprintf(stderr, "Send subscribe to '%s'\n", dest);
    char buf[64];
    sprintf(buf, "SUBSCRIBE\ndestination:%s\n\n", dest);
    int n = write(sockfd, buf, strlen(buf) + 1);
    if (n != strlen(buf) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int send_send(int sockfd, char *topic, char *content) {
    fprintf(stderr, "Send send to topic '%s'\n", topic);
    char buf[128];
    sprintf(buf, "SEND\ntopic:%s\n\n%s\n\n", topic, content);
    int n = write(sockfd, buf, strlen(buf) + 1);
    if (n != strlen(buf) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    } 
    return 0;
}

int send_disconnect(int sockfd, char *name) {
    fprintf(stderr, "Send disconnect '%s'\n", name);
    char buf[] = "DISCONNECT\n\n";
    int n = write(sockfd, buf, strlen(buf) + 1);
    if (n != strlen(buf) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    } 
    return 0;
}

int expect_connected(int sockfd) {
    char buf[1024];
    int ret;
    ret = read_from_server(sockfd, buf, 1024);
    return ret == 0 && 0 == strcmp(buf, "CONNECTED\n\n");
}

int read_from_server(int sockfd, char *buf, size_t buflen) {
    int nbytes, pos;
    char c;

    pos = 0;
    while (pos < buflen) {
        nbytes = read(sockfd, &c, 1); 
        if (nbytes == 0) {
            fprintf(stderr, "EOF\n");
            return -1;
        }
        buf[pos++] = c;
        // end of command
        if (c == '\0') {
            break;
        }
    }

    if (pos == buflen && buf[pos-1] != '\0') {
        fprintf(stderr, "Server sent too much\n");
        return -1;
    } else {
        return 1;
    }
}

int connect_to_server(struct hostent *server, int port) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr_list[0], 
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sockfd,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr))
        < 0) { 
        error("ERROR connecting");
    }
    return sockfd;
}
