/*
** whatsmyip.c
** A simple HTTP server which returns client's IP address.
** Runs on port 8000 by default
**
** By Vivek Agarwal <me →AT→ vivek →DOT→ im>
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8000"
#define BACKLOG 10 // max number of pending connections in queue
#define MAXBUFSIZE 5000 // maximum recv buffer size

void *get_in_addr(struct sockaddr *sa);

int main(void) {

    int listen_fd; // for socket on which we listen for connection
    int new_fd; // file descriptor for new connection
    struct addrinfo hints, *servinfo, *p;
    int err_code;
    int yes = 1; // for setsockopt() use
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char user_ip[INET6_ADDRSTRLEN]; // for storing user's IP address
    char buf[MAXBUFSIZE]; // for storing
    int numbytes;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // IP version agnostic
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Use own IP

    // getaddrinfo() returns 0 on success otherwise the error code
    if((err_code = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err_code));
    }

    // loop through all the results in addrinfo linked list
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // get the socket descriptor to listen on
        listen_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listen_fd == -1) {
            perror("socket() error");
            continue;
        }
        // try to reuse the port if it is blocked
        if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt() error");
            exit(1);
        }
        // now bind to the socket so that we can listen on it
        if(bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_fd);
            perror("bind() error");
            continue;
        }
        // if the code above succeeded then break out
        // now we have a socket fd and it has been bind()ed
        break;
    }

    // if p reached the end of linked list, it means the loop completed
    // and error(s) occured while in the loop
    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // not needed anymore

    // start listening
    if(listen(listen_fd, BACKLOG) == -1) {
        perror("listen() error");
        exit(1);
    }

    printf("running on port %s...\n", PORT);

    while(1) {
        sin_size = sizeof(their_addr);
        new_fd = accept(listen_fd, (struct sockaddr *) &their_addr, &sin_size);
        if(new_fd == -1) {
            perror("accept() error");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  user_ip, INET6_ADDRSTRLEN);

        if((numbytes = recv(new_fd, buf, MAXBUFSIZE-1, 0)) == -1) {
            perror("recv() error");
            exit(1);
        }
        buf[numbytes] = '\0';
        printf("Received from %s:\n%s\n", user_ip, buf);

        if(send(new_fd, "HTTP/1.0 200 OK\nContent-Type: text/html\n\n", 41, 0) == -1) {
            perror("send() error");
        }
        if(send(new_fd, user_ip, strlen(user_ip), 0) == -1) {
            perror("send() error");
        }

        close(new_fd);
    }
    close(listen_fd);
    return 0;

}

// get in_addr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
