#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void skip_empty_lines(const char *ptr) {
  while (*ptr == '\r' || *ptr == '\n') {
    ptr++;
  }
}

int parse_http_request(const char *raw_request, HttpRequest *request) {
  memset(request, 0, sizeof(HttpRequest)); // обнуление

  char line[1024];
  const char *rr_ptr = raw_request; // initially points to string start
                                    // нужно для итерации по raw_request
                                    // без её изменения

  int i = 0;
  while (*rr_ptr != '\r' && *rr_ptr != '\n' && *rr_ptr != '\0') {
    line[i++] = *rr_ptr++;
  }

  line[i] = '\0';

  sscanf(line, "%7s %255s", request->method, request->path);
  printf("method -> %s\tpath -> %s\n", request->method, request->path);

  skip_empty_lines(rr_ptr);
  // while (*rr_ptr == '\r' || *rr_ptr == '\n') {
  //   rr_ptr++;
  // }

  // headers
  request->headers_count = 0;
  while (*rr_ptr != '\0' && *rr_ptr != '\r' && *rr_ptr != '\n') {
    if (request->headers_count > MAX_HEADERS)
      break;

    // header key
    i = 0;
    while (*rr_ptr != ':' && *rr_ptr != '\0') {
      line[i++] = *rr_ptr++;
    }
    line[i] = '\0';

    strncpy(request->headers[request->headers_count][0], line,
            MAX_HEADER_NAME_LEN);
    if (*rr_ptr == ':')
      rr_ptr++;
    while (*rr_ptr == ' ')
      rr_ptr++;

    // header value
    i = 0;
    while (*rr_ptr != '\n' && *rr_ptr != '\r' && *rr_ptr != '\0') {
      line[i++] = *rr_ptr++;
    }
    line[i] = '\0';

    strncpy(request->headers[request->headers_count][1], line,
            MAX_HEADER_VALUE_LEN);

    request->headers_count++;

    skip_empty_lines(rr_ptr);
    // while (*rr_ptr == '\r' || *rr_ptr == '\n')
    //   rr_ptr++;
  }

  skip_empty_lines(rr_ptr);
  // while (*rr_ptr == '\r' || *rr_ptr == '\n')
  //   rr_ptr++;

  if (*rr_ptr != '\0') {
    // body
    size_t body_len = strlen(rr_ptr);
    request->body = malloc(body_len + 1);
    if (request->body) {
      strcpy(request->body, rr_ptr);
      request->body_len = body_len;
    }
  }

  return 0;
}

void free_http_request(HttpRequest *req) {
  if (req->body) {
    free(req->body);
    req->body = NULL;
  }
}
