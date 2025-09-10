#include "websockets.h"
#include "http.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>

int is_websocket_handshake(HttpRequest *req) {
  const char *upgrade = find_header(req, "upgrade");
  const char *connection = find_header(req, "connection");
  const char *ws_version = find_header(req, "sec-websocket-version");
  const char *ws_key = find_header(req, "sec-websocket-key");

  return (upgrade && strcasecmp(upgrade, "websocket") == 0 && 
          connection && strcasestr(connection, "upgrade") != NULL &&
          ws_key && ws_version && strcasecmp(ws_version, "13"));
}
