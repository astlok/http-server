#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "tcp.h"
#include "http.h"

int main() {
    int socket_listen_fd,
            portno = 80,
            client_len,
            socket_connection_fd,
            kq,
            new_events;
    struct kevent change_event,
            event[32],
            kekevent[4];
    struct sockaddr_in serv_addr,
            client_addr;

    if (((socket_listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)) {
        perror("ERROR opening socket");
        exit(1);
    }

    if (fcntl(socket_listen_fd, F_SETFL, O_NONBLOCK) < 0) {
        return -1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(socket_listen_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    // Start listening.
    listen(socket_listen_fd, 3);
    client_len = sizeof(client_addr);

    kq = kqueue();

    EV_SET(&change_event, socket_listen_fd, EVFILT_READ , EV_ADD, 0, 0, 0);

    if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1) {
        perror("kevent");
        exit(1);
    }

    for (;;) {
        new_events = kevent(kq, NULL, 0, event, 10, NULL);
        if (new_events == -1) {
            perror("kevent");
            exit(1);
        }

        char *path = NULL;
        char *header = NULL;
        for (int i = 0; new_events > i; i++) {
            printf("amount of new events: %d\n", new_events);
            int event_fd = event[i].ident;

            if (event[i].flags & EV_EOF) {
                printf("Client has disconnected\n");
                close(event_fd);
            }
            if (event_fd == socket_listen_fd) {
                printf("New connection coming in...\n");

                socket_connection_fd = accept_socket(event_fd, (struct sockaddr *) &client_addr,
                                                     (socklen_t *) &client_len);
                if (socket_connection_fd < 0) {
                    perror("Accept socket error");
                }
                EV_SET(&change_event, socket_connection_fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, NULL);
                if (kevent(kq, &change_event, 1, NULL, 0, NULL) < 0) {
                    perror("kevent error");
                }
            } else if (event[i].filter == EVFILT_READ) {
                char buf[1024];
                size_t bytes_read = recv(event_fd, buf, sizeof(buf), 0);
                printf("read %zu bytes\n", bytes_read);

                if (get_header_and_path(buf, bytes_read, &path, &header) < 0) {
                    perror("parse error");
                    close(event_fd);
                }
                if (send_response(event_fd, path, header) < 0) {
                    EV_SET(&change_event, event_fd, EVFILT_READ, EV_ENABLE | EV_ADD | EV_ONESHOT , 0, 0, NULL);
                    printf("error send\n");
                    close(event_fd);
                    if (kevent(kq, &change_event, 1, NULL, 0, NULL) < 0) {
                        perror("kevent error");
                    }
                } else {
                    close(event_fd);
                    printf("connection close\n");
                }
            }
        }
    }
};
