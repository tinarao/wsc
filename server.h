#ifndef SERVER_H
#define SERVER_H

#include "http.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MSG_SIZE 4096
#define LISTEN_BACKLOG 69

int start_http_server(int port);
int handle_request(int conn_fd);

#endif
