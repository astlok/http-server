cmake_minimum_required(VERSION 3.8.4)
project(http_server C)

include_directories(http_server C include)
add_executable(http_server
        src/main.c
        src/tcp.c
        src/http.c
        src/utils.c)

