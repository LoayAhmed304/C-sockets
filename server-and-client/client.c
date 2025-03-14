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

#define PORT "3030"
#define MAX_DATA_SIZE 2048 // maximum size of data we can get at once

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
int main(int argc, char **argv)
{
    if (argc != 3)
    {
        perror("usage: ./client hostname message_to_send\n");
        exit(1);
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(argv[1], PORT, &hints, &res);
    if (status != 0)
    {
        perror("Error getaddrinfo");
        return 1;
    }
    struct addrinfo *p;
    int sockfd;
    int yes = 1;
    for (p = res; p != NULL; p = p->ai_next)
    {

        // try to create the socket
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("error with sockfd\n");
            continue;
        }
        // socket matched successfully with the addrinfo

        /// try to configure the socket to free the port immediately when aborted
        int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (status == -1)
        {
            perror("error with setsockopt\n");
            exit(1); // its error is an OS error, not bad socket error
        }
        // successful set socket's configuration

        // we don't need to call bind(), because we don't care from what port we send the requests
        status = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (status == -1)
        {
            close(sockfd);
            perror("error connecting");
            continue;
        }
        // done connecting

        // if passed all pipeline, break
        break;
    }
    if (p == NULL)
    { // case I didn't "break", but finished the for loop
        perror("Couldn't connect");
        return 1;
    }
    char IP[INET6_ADDRSTRLEN];
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), IP, sizeof IP);
    printf("Connected to IP: %s, and our IP is: UNKNOWN\n", IP);

    freeaddrinfo(res); // all done with this struct

    char data[MAX_DATA_SIZE];
    int numBytes = recv(sockfd, &data, MAX_DATA_SIZE - 1, 0);
    if (numBytes == -1)
    {
        perror("Failed to receive data");
        close(sockfd);
        exit(1);
    }
    data[numBytes] = '\0'; // add null terminator to the data received

    printf("Data sent to us from the server: %s\n", data);

    char *msg = argv[2];
    int bytesSent = send(sockfd, msg, strlen(msg), 0);
    if (bytesSent == -1)
    {
        perror("Failed to send\n");
    }
    close(sockfd);
    return 0;
}