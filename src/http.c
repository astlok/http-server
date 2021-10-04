#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "http.h"
#include "utils.h"

int get_header_and_path(char *string_buf, size_t bytes_read, char **path, char **header) {
    int is_header = 0;
    int header_length = 0;
    int path_length = 0;
    for (int i = 0; i < bytes_read; ++i) {
        if ((string_buf[i] == 32 || string_buf[i] == 10) && is_header) {
            break;
        }
        if (string_buf[i] == 32) {
            is_header = 1;
            continue;
        }
        if (!is_header) {
            header_length++;
        } else {
            path_length++;
        }
    }
    *header = (char *) malloc(sizeof(char) * (header_length + 1));
    *path = (char *) malloc(sizeof(char) * (path_length + 1));

    strlcpy(*header, string_buf, header_length + 1);
    string_buf += sizeof(char) * (header_length + 1);
    strlcpy(*path, string_buf, path_length + 1);
    string_buf -= sizeof(char) * (header_length + 1);
    return 0;
}

FILE *get_response(char *path, char *header, char **reply) {


    char *date;
    date = (char *) malloc(sizeof(char) * 64);
    if (!date) {
        return NULL;
    }
    get_date(&date);
    FILE *in_file = NULL;
    if (strcasestr(header, "HEAD") != NULL || strcasestr(header, "GET") != NULL) {
        int code = get_code(header, &path, 1);

        in_file = fopen(path + sizeof(char), "r");
        int file_exist = 0;
        if (in_file != NULL) {
            file_exist = 1;
        }
        long sz = 0;
        if (file_exist) {
            fseek(in_file, 0L, SEEK_END);
            sz = ftell(in_file);
            fseek(in_file, 0L, SEEK_SET);
        }
        char *code_name;
        char *content_type;

        code_name = (char *) malloc(sizeof(char) * 64);
        content_type = (char *) malloc(sizeof(char) * 256);
        get_code_name(code, &code_name);
        if (file_exist) {
            get_content_type(path, &content_type);
        }

        if (file_exist) {
            sprintf(*reply, "HTTP/1.1 %d %s\n"
                            "Date: %s\n"
                            "Server: Apache/2.2.3\n"
                            "Content-Type: %s\n"
                            "Content-Length: %li\n"
                            "Connection: close\n"
                            "\n", code, code_name, content_type, date, sz);
        } else {
            sprintf(*reply, "HTTP/1.1 %d %s\n"
                            "Date: %s\n"
                            "Server: Apache/2.2.3\n"
                            "Connection: close\n"
                            "\n", code, code_name, date);
        }

        free(code_name);
        free(content_type);

    } else {
        sprintf(*reply, "HTTP/1.1 405 Method Not Allowed\n"
                        "Date: %s\n"
                        "Server: Apache/2.2.3\n"
                        "Connection: close\n"
                        "\n", date);
    }

    free(date);


    return in_file;
}

int get_code(char *header, char **path, int file_exist) {
    file_exist = 0;
    char *out_of_root = strstr(header, "../");
    if (out_of_root) {
        return 403;
    }

    if (access(*(path) + 1, F_OK) == 0) {
        file_exist = 1;
    }
    if ((*path)[strlen(*path) - 1] == '/') {
        *path = (char *) realloc(*path, strlen(*path) + 11 * sizeof(char));
        sprintf(*path, "%s%s", *path, "index.html");
        if (access(*(path) + 1, F_OK) == 0) {
            file_exist = 1;
        } else {
            return 403;
        }
    }
    if (file_exist) {
        return 200;
    }
    return 404;
}

void get_code_name(int code, char **code_name) {
    switch (code) {
        case 200:
            strcpy(*code_name, "OK");
            break;
        case 403:
            strcpy(*code_name, "Forbidden");
            break;
        default:
            strcpy(*code_name, "Not Found");
    }
}

void get_content_type(char *path, char **content_type) {
    char extension[64];
    for (int i = 0; i < strlen(path); ++i) {
        if (path[i] == '.') {
            strcpy(extension, path + sizeof(char) * i);
            break;
        } else {
            continue;
        }
    }
    if (strcasestr(extension, "html") != NULL) {
        strcpy(*content_type, "text/html");
    } else if (strcasestr(extension, "js") != NULL) {
        strcpy(*content_type, "application/javascript");
    } else if (strcasestr(extension, "jpg") != NULL) {
        strcpy(*content_type, "image/jpeg");
    } else if (strcasestr(extension, "jpeg") != NULL) {
        strcpy(*content_type, "text/jpeg");
    } else if (strcasestr(extension, "png") != NULL) {
        strcpy(*content_type, "text/png");
    } else if (strcasestr(extension, "gif") != NULL) {
        strcpy(*content_type, "text/gif");
    } else if (strcasestr(extension, "swf") != NULL) {
        strcpy(*content_type, "application/x-shockwave-flash");
    } else if (strcasestr(extension, "txt") != NULL) {
        strcpy(*content_type, "text/plain");
    }
}
