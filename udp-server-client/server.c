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

#define MYPORT "3030" // server port (where users will connect)
#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // my ip
    int sockfd;

    int status = getaddrinfo(NULL, MYPORT, &hints, &servinfo);
    if (status != 0)
    {
        perror("error get addrinfo\n");
        return 1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
        {
            perror("socket creation error\n");
            continue;
        }
        status = bind(sockfd, p->ai_addr, p->ai_addrlen);
        if (status == -1)
        {
            close(sockfd);
            perror("error binding");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        perror("error finding good result");
        freeaddrinfo(servinfo);
        return 2;
    }
    char ss[INET6_ADDRSTRLEN];
    // char *IPvv4 = inet_ntop(p->ai_family, get_in_addr(p->ai_addr), ss, sizeof ss);

    // printf("Server: %s", IPvv4);
    freeaddrinfo(servinfo);

    printf("listener: Waiting to recvfrom...\n");

    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    char buffer[101];
    char s[INET6_ADDRSTRLEN];
    while (1)
    {

        int numBytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
        if (numBytes == -1)
        {
            perror("Error with recvfrom man");
            exit(1);
        }
        char *IPv4 = inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        if (numBytes != -1)
        {
            printf("listner: got packet from %s\n", s);
            printf("listner: packet is %d bytes long\n", numBytes);
            buffer[numBytes] = '\0';
            printf("listener: packet contains: %s\n", buffer);
            sendto(sockfd, "Message received", 16, 0, (struct sockaddr *)&their_addr, addr_len);
        }
    }
    close(sockfd);
}