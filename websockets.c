#include "websockets.h"
#include "http.h"
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int is_websocket_handshake(HttpRequest *req) {
  const char *upgrade = find_header(req, "upgrade");
  const char *connection = find_header(req, "connection");
  const char *ws_version = find_header(req, "sec-websocket-version");
  const char *ws_key = find_header(req, "sec-websocket-key");

  return (upgrade && strcasecmp(upgrade, "websocket") == 0 && connection &&
          strcasestr(connection, "upgrade") != NULL && ws_key && ws_version &&
          strcasecmp(ws_version, "13"));
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
