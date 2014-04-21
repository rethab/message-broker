#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>

void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr_list[0], 
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    // connect
    printf("Send connect\n");
    char cmd1[] = "CONNECT\nlogin:foo\n\n";
    n = write(sockfd, cmd1, strlen(cmd1) + 1);
    if (n != strlen(cmd1) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    } 

    // subscribe
    printf("Send subscribe\n");
    char cmd2[] = "SUBSCRIBE\ndestination:stocks\n\n";
    n = write(sockfd, cmd2, strlen(cmd2) + 1);
    if (n != strlen(cmd2) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    } 

    // send
    printf("Send send\n");
    char cmd3[] = "SEND\ntopic:stocks\n\nhello, world\n\n";
    n = write(sockfd, cmd3, strlen(cmd3) + 1);
    if (n != strlen(cmd3) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    } 

    // disconnect
    printf("Send disconnect\n");
    char cmd4[] = "DISCONNECT\n\n";
    n = write(sockfd, cmd4, strlen(cmd4) + 1);
    if (n != strlen(cmd4) + 1) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
    } 

    int nbytes;
    char buf[1024];
    while (1) {
        nbytes = read(sockfd, &buf, 1023);
        if (nbytes == -1) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (nbytes == 0) {
            printf("Done\n");
            break;
        }

        buf[nbytes] = '\0';
        printf("Read From Server: '%s'\n", buf);
    }
    return 0;
}
