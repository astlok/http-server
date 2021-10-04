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
//    int set = 1;
//    setsockopt(event_fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    if (fcntl(new_socket, F_SETFL, O_NONBLOCK) < 0) {
        return -1;
    }
    return new_socket;
}

int send_response(int event_fd, char *path, char *header) {
    char *reply;
    reply = (char *) malloc(sizeof(char) * 1024);
    FILE *in_file = get_response(path, header, &reply);
    send(event_fd, reply, strlen(reply), 0);

    if (in_file && strcasestr(header, "HEAD") == NULL) {
//        while (fgets(data, 1024, in_file) != NULL) {
//            if (write(event_fd, data, strlen(data)) == -1) {
//                perror("[-]Error in sending file.");
//            }
//            bzero(data, 1024);
//        }
//        fclose(in_file);
//            send_file(event_fd, in_file);1
        long data = 0;
        sendfile(fileno(in_file), event_fd, 0, (off_t *) &data, NULL, 0);
        fclose(in_file);
    }
    return 0;
}

int senddata(int sock, void *buf, int buflen) {
    unsigned char *pbuf = (unsigned char *) buf;

    while (buflen > 0) {
        int num = send(sock, pbuf, buflen, 0);

        pbuf += num;
        buflen -= num;
    }

    return 1;
}

//int sendlong(int sock, long value) {
//    value = htonl(value);
//    return senddata(sock, &value, sizeof(value));
//}

int send_file(int sock, FILE *f) {
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0L, SEEK_SET);
    if (filesize == EOF)
        return 0;
//    if (!sendlong(sock, filesize))
//        return 0;
    if (filesize > 0) {
        char buffer[1024];
        do {
            size_t num = min(filesize, sizeof(buffer));
            num = fread(buffer, sizeof(char), num, f);
            if (num < 1)
                return 0;
            if (!senddata(sock, buffer, num))
                return 0;
            filesize -= num;
        } while (filesize > 0);
    }
    return 1;
}

size_t min(size_t first, size_t second) {
    if (first < second) {
        return first;
    } else {
        return second;
    }
}

