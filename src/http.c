#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "http.h"
#include "utils.h"
#include <sys/stat.h>

int get_header_and_path(char *string_buf, size_t bytes_read, char **path, char **header) {
    int is_header = 0;
    int header_length = 0;
    int path_length = 0;

    for (int i = 0; i < bytes_read; ++i) {
        if ((string_buf[i] == ' ' || string_buf[i] == '\n') && is_header) {
            break;
        }
        if (string_buf[i] == ' ') {
            is_header = 1;
            continue;
        }
        if (string_buf[i] == '?') {
            break;
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
    *path = urlDecode(*path);
    return 0;
}

char *urlDecode(const char *str) {
    int d = 0; /* whether or not the string is decoded */

    char *dStr = malloc(strlen(str) + 1);
    char eStr[] = "00"; /* for a hex code */

    strcpy(dStr, str);

    while (!d) {
        d = 1;
        int i; /* the counter for the string */

        for (i = 0; i < strlen(dStr); ++i) {

            if (dStr[i] == '%') {
                if (dStr[i + 1] == 0)
                    return dStr;

                if (isxdigit(dStr[i + 1]) && isxdigit(dStr[i + 2])) {

                    d = 0;

                    /* combine the next to numbers into one */
                    eStr[0] = dStr[i + 1];
                    eStr[1] = dStr[i + 2];

                    /* convert it to decimal */
                    long int x = strtol(eStr, NULL, 16);

                    /* remove the hex */
                    memmove(&dStr[i + 1], &dStr[i + 3], strlen(&dStr[i + 3]) + 1);

                    dStr[i] = x;
                }
            }
        }
    }

    return dStr;
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


        int file_exist = 0;
        if (is_regular_file(path + 1)) {
            file_exist = 1;
        }
        if (file_exist) {
            in_file = fopen(path + sizeof(char), "r");
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

        if (file_exist && strcasestr(header, "HEAD") == NULL) {
            sprintf(*reply, "HTTP/1.1 %d %s\r\n"
                            "Date: %s\r\n"
                            "Server: Apache/2.2.3\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %li\r\n"
                            "Connection: close\r\n"
                            "\r\n", code, code_name, date, content_type, sz);
        } else if (file_exist && strcasestr(header, "HEAD") != NULL) {
            sprintf(*reply, "HTTP/1.1 %d %s\r\n"
                            "Date: %s\r\n"
                            "Server: Apache/2.2.3\r\n"
                            "Content-Type: %s\r\n"
                            "Content-Length: %li\r\n"
                            "Connection: close"
                            "\r\n\r\n", code, code_name, date, content_type, sz);
        } else {
            sprintf(*reply, "HTTP/1.1 %d %s\r\n"
                            "Date: %s\r\n"
                            "Server: Apache/2.2.3\r\n"
                            "Connection: close\r\n"
                            "\r\n", code, code_name, date);
        }
        printf("%s\n", *reply);
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
    char *out_of_root = strstr(*path, "../");
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
            int size = strlen(*path); //Total size of string
            (*path)[size - 11] = '\0';
            if (is_regular_file(*(path) + 1)) {
                return 404;
            }
            return 403;
        }
    }
    if (file_exist) {
        return 200;
    }
    return 404;
}

int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
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
        strcpy(*content_type, "image/jpeg");
    } else if (strcasestr(extension, "png") != NULL) {
        strcpy(*content_type, "image/png");
    } else if (strcasestr(extension, "gif") != NULL) {
        strcpy(*content_type, "image/gif");
    } else if (strcasestr(extension, "swf") != NULL) {
        strcpy(*content_type, "application/x-shockwave-flash");
    } else if (strcasestr(extension, "txt") != NULL) {
        strcpy(*content_type, "text/plain");
    } else if (strcasestr(extension, "css") != NULL) {
        strcpy(*content_type, "text/css");
    }
}
