#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

void skip_empty_lines(const char **ptr) {
  while (**ptr == '\r' || **ptr == '\n') {
    (*ptr)++;
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

  skip_empty_lines(&rr_ptr);

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

    skip_empty_lines(&rr_ptr);
  }

  skip_empty_lines(&rr_ptr);

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

// Responses

int build_http_response(const HttpResponse *res, char *buf, size_t buf_size) {
  // Первая строка ответа - протокол, статускод и статус-текст
  // written - счётчик записанных байт
  int written = snprintf(buf, buf_size, "HTTP/1.1 %d %s\r\n", res->status_code,
                         res->status_text);

  // запись заголовков ответа
  for (size_t i = 0; i < res->headers_count; i++) {
    written += snprintf(buf + written, buf_size - written, "%s: %s\r\n",
                        res->headers[i][0], res->headers[i][1]);
  }

  // разделитель между заголовками и телом
  written += snprintf(buf + written, buf_size - written, "\r\n");

  if (res->body && res->body_len > 0) {
    // check how much bytes we can SAFELY copy
    size_t to_copy = (buf_size - written - 1) < res->body_len
                         ? (buf_size - written - 1)
                         : res->body_len;

    // copy `to_copy` bytes from res->body to buf+written
    memcpy(buf + written, res->body, to_copy);
    written += to_copy;
  }

  buf[written] = '\0';
  // written += snprintf(buf + written, buf_size - written, "\r\n");
  return written;
}

void add_header(HttpResponse *res, const char *name, const char *value) {
  if (res->headers_count < MAX_HEADERS) {
    strncpy(res->headers[res->headers_count][0], name, MAX_HEADER_NAME_LEN);
    strncpy(res->headers[res->headers_count][1], value, MAX_HEADER_VALUE_LEN);
    res->headers_count++;
  }
}

void free_http_response(HttpResponse *response) {
  if (response->body) {
    free(response->body);
    response->body = NULL;
  }

  if (response->ws_accept_hash) {
    free(response->ws_accept_hash);
    response->ws_accept_hash = NULL;
  }
}

void create_ok_response(HttpResponse *response) {
  memset(response, 0, sizeof(HttpResponse));
  response->status_code = 200;
  response->status_text = "OK";

  add_header(response, "Content-Type", "text/plain");
  add_header(response, "Connection", "close");

  response->body = "Recieved!\n";
  response->body_len = 11;

  char content_length_header[20];
  snprintf(content_length_header, sizeof(content_length_header), "%zu",
           response->body_len);
  add_header(response, "Content-Length", content_length_header);
}

void create_err_response(HttpResponse *response, int status, char *status_str,
                         const char *body) {
  memset(response, 0, sizeof(HttpResponse));
  response->status_code = status;
  response->status_text = status_str;

  add_header(response, "Content-Type", "text/plain");
  add_header(response, "Connection", "close");

  response->body = strdup(body);
  if (response->body) {
    response->body_len = strlen(response->body);
  }

  char content_length_header[20];
  snprintf(content_length_header, sizeof(content_length_header), "%zu",
           response->body_len);
  add_header(response, "Content-Length", content_length_header);
}

const char *find_header(HttpRequest *req, char *name) {
  if (req->headers_count < 1)
    return NULL;

  for (size_t i = 0; i < req->headers_count; i++) {
    if (strcasecmp(req->headers[i][0], name) == 0) {
      return req->headers[i][1];
    };
  };

  return NULL;
}

// Websocket-related

int is_websocket_handshake(HttpRequest *req) {
  const char *upgrade = find_header(req, "upgrade");
  const char *connection = find_header(req, "connection");
  const char *ws_version = find_header(req, "sec-websocket-version");
  const char *ws_key = find_header(req, "sec-websocket-key");

  // printf("conn upgrade: %s\n", strcasestr(connection, "upgrade"));
  // printf("version_found: %d\n", strcasecmp(ws_version, "13"));
  // printf("exact version: %d\n", atoi(ws_version));

  return (upgrade && strcasecmp(upgrade, "websocket") == 0 && connection &&
          strcasestr(connection, "upgrade") != NULL && ws_key && ws_version &&
          atoi(ws_version) == 13);
}

int make_ws_accept_hash(HttpResponse *res, const char *sec_ws_key) {
  if (sec_ws_key == NULL)
    return -1;

  char *magic_str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  size_t key_len = strlen(sec_ws_key);
  size_t magic_len = strlen(magic_str);

  char *hash_base = malloc(key_len + magic_len + 1);
  if (hash_base == NULL) {
    perror("Failed to allocate mem for hash_base");
    return -1;
  }

  strcpy(hash_base, sec_ws_key);
  strcat(hash_base, magic_str);

  // sha1
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1((unsigned char *)hash_base, strlen(hash_base), hash);

  // base64
  BIO *b64, *bio;
  BUF_MEM *bptr = NULL;
  char *buf = NULL;

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);
  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

  BIO_write(bio, hash, SHA_DIGEST_LENGTH);
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &bptr);

  buf = (char *)malloc(bptr->length + 1);
  if (buf != NULL) {
    memcpy(buf, bptr->data, bptr->length);
    buf[bptr->length] = '\0';
  }

  BIO_free_all(bio);
  free(hash_base);

  res->ws_accept_hash = buf;
  return 0;
}

int build_ws_handshake_response(HttpResponse *res, const char *sec_ws_key) {
  memset(res, 0, sizeof(HttpResponse));
  res->status_code = 101;
  res->status_text = "Switching Protocols";

  // set res->accept_hash
  if (make_ws_accept_hash(res, sec_ws_key) == -1) {
    perror("Failed to calculate accept hash");
    return -1;
  }

  add_header(res, "connection", "upgrade");
  add_header(res, "upgrade", "websocket");
  add_header(res, "sec-websocket-accept", res->ws_accept_hash);

  return 0;
}

