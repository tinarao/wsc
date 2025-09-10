#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

// 1. В handle_request(int conn_fd) проверяем,
//         является ли запрос Websockets: Update.
//         Критерии:
//         1. GET
//         2. Заголовок Upgrade: websocket
//         3. Заголовок Connection: upgrade
//         4. Заголовок Sec-WebSocket-Version: 13
//         5. Заголовок Sec-WebSocket-Key должен присутствовать
// 2. Если да:
//         1. Отдаём 101
//         2. Вычисляем Sec-WebSocket-Accept: берем Sec-WebSocket-Key, добавляем GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
//         3. Вычисляем SHA-1 хэш, затем base64 кодируем.
//         4. Отправляем ответ и переводим в режим websocket.
//         5. После успешного handshake, нужно читать и писать WebSocket-фреймы. 
//         6. Реализуем парсинг и создание ws-фреймов.
#include "http.h"

// 1 is yes, else 0
int is_websocket_handshake(HttpRequest *req);

int build_ws_handshake_response(HttpResponse *res, const char* sec_ws_key);

int make_ws_accept_hash(HttpResponse *res, const char *sec_ws_key);
#endif
