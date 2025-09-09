#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define MAX_HEADERS 20
#define MAX_PATH_LEN 20
#define MAX_HEADER_VALUE_LEN 256
#define MAX_HEADER_NAME_LEN 32 

// Requests

typedef struct {
    char method[8];
    char path[MAX_PATH_LEN];
    char headers[MAX_HEADERS][2][MAX_HEADER_VALUE_LEN];
    size_t headers_count;
    size_t body_len;
    char* body;
} HttpRequest;

int parse_http_request(const char* req, HttpRequest* out);
void free_http_request(HttpRequest* req);

// Responses

typedef struct {
    int status_code;
    char headers[MAX_HEADERS][2][MAX_HEADER_VALUE_LEN];
    size_t headers_count;
    size_t body_len;
    char* status_text;
    char* body;
} HttpResponse;

int build_http_response(const HttpResponse* response, char* buf, size_t buf_size);
void add_header(HttpResponse* response, const char* name, const char* value);
void free_http_response(HttpResponse* response);

void create_ok_response(HttpResponse* response);
void create_err_response(HttpResponse *response, int status, char* status_str, const char* body);

#endif
