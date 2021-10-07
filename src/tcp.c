#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "http.h"
#include "tcp.h"

int accept_socket(int event_fd, struct sockaddr *client_addr, socklen_t *client_len) {
    int new_socket = accept(event_fd, client_addr, client_len);
    if (new_socket == -1) {
        return -1;
    }
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
        return -1;
    }
    return new_socket;
}

int send_response(int event_fd, char *path, char *header) {
    char *reply;
    reply = (char *) malloc(sizeof(char) * 1024);
    FILE *in_file = get_response(path, header, &reply);
    if (send(event_fd, reply, strlen(reply), 0) < 0) {
        return -1;
    }

    if (in_file && strcasestr(header, "HEAD") == NULL) {
        long data = 0;
        if (sendfile(fileno(in_file), event_fd, 0, (off_t *) &data, NULL, 0) < 0) {
            return -1;
        }
        fclose(in_file);
    }
    return 0;
}
