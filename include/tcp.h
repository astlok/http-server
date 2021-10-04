#ifndef HTTP_SERVER_TCP_H
#define HTTP_SERVER_TCP_H

#endif //HTTP_SERVER_TCP_H

int accept_socket(int event_fd, struct sockaddr *client_addr, socklen_t * client_len);

int send_response(int event_fd, char *path, char *header);

