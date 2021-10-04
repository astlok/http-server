#ifndef HTTP_SERVER_HTTP_H
#define HTTP_SERVER_HTTP_H

#endif //HTTP_SERVER_HTTP_H

int get_header_and_path(char *string_buf, size_t bytes_read, char **path, char **header);

FILE *get_response(char *path, char *header, char **reply);

int get_code(char *header, char **path, int file_exist);

void get_code_name(int code, char **code_name);

void get_content_type(char *path, char **content_type);
