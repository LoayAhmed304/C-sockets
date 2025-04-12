#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT "3031" // server port (where users will connect)
#define SERVERPORT "3030"
#define MAXBUFLEN 100

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int status;
    int numBytes;

    if (argc != 3)
    {
        fprintf(stderr, "usage: ./client hostname message");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    status = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo);
    if (status != 0)
    {
        fprintf(stderr, "error with getaddrinfo");
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            fprintf(stderr, "socket creation error");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "error creating socket from all results");
        return 2;
    }
    numBytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, p->ai_addr, p->ai_addrlen);
    if (numBytes == -1)
    {
        fprintf(stderr, "error sending");
        exit(1);
    }
    freeaddrinfo(servinfo);
    printf("client sent: %s, to %s\n", argv[2], argv[1]);
    return 0;
}