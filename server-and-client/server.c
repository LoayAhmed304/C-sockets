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

#define PORT "3030" // Port users will connect to
#define BACKLOG 20  // How many pending connection queue will hold

// to ensure all finished child processes are cleaned up;
void sigchld_handler(int s)
{

    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0) // cleans all zombie child processes, -1 means wait for ANY child process.
        ;

    errno = saved_errno;
}

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
    int sockfd, new_fd; // listen on sockfd, socket returned from connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector address information

    int yes = 1;
    int rv;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // My own IP (because its the server)

    if (rv = getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "Error happened while grapping addr info: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    p = (struct addrinfo *)servinfo;
    while (p != NULL)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            fprintf(stderr, "Error creating socket: %s\n", gai_strerror(sockfd));
            p = p->ai_next;
            continue;
        }

        // Configure the socket to free the port immediately when the server is closed
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt error!!!!");
            exit(1);
        }

        // bind it to the port & ip (in ai_addr)
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("Error while binding");
            p = p->ai_next;
            continue;
        }
        break;
    }
    // Now we have the socket created on sockfd and binded successfully...
    // free the address
    freeaddrinfo(servinfo);
    if (p == NULL)
    {
        fprintf(stderr, "Couldn't create socket & bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        fprintf(stderr, "server can't listen\n");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
    printf("Waiting for connections...");

    char s[INET6_ADDRSTRLEN]; // to hold the IP (v6 or v4)
    socklen_t sin_size;

    while (1) // main listening loop
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("can't accept");
            continue;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("Server got connection from: %s\n", s);
        if (!fork()) // if fork() == 0 (child process)
        {
            close(sockfd); // child doesn't need the listener, it'll be closed for this process only (child)
            // Now let's send something
            char *msg = "Welcome to my socket server!";
            if (send(new_fd, msg, sizeof msg, 0) == -1)
            {
                perror("failed to send");
            }
            while (1)
            {
                char buffer[1024];
                int bytes_received = recv(new_fd, buffer, sizeof buffer - 1, 0); // receive from my socket and store in buffer
                if (bytes_received <= 0)
                {
                    perror("client disconnected or recv error");
                    break;
                }
                buffer[bytes_received] = '\0'; // null terminator
                printf("Client %d says: %s\n", getpid(), buffer);

                // now send the message back
                if (send(new_fd, buffer, bytes_received, 0) == -1)
                {
                    perror("failed to send from the server!!");
                    break;
                }
            }
            close(new_fd);
            exit(0); // terminate the child process so it doesn't continue executing the code meant for the parent
        }
        close(new_fd); // parent closes this, it's closed only in the parent process but not in child
    }

    return 0;
}
