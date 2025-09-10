#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t fin;           // 1 bit, указывает, является ли фрейм последним в сообщении
    uint8_t opcode;        // 4 bit, определяет тип фрейма
    uint8_t mask;          // 1 bit, указывает, замаскированы ли данные (клиент - 1, сервер - 0)
    uint8_t masking_key[4];  // 32 bit, используется для маскировки данных
                           // должен присутствовать если Mask=1
    uint64_t payload_len;  
    uint8_t *payload;
} WSFrame;

// returns amount of written bytes
int read_ws_frame(int conn_fd, WSFrame *frame);

// returns amount of written bytes
int send_ws_frame(int conn_fd, const uint8_t *payload, size_t payload_len, uint8_t opcode);

#endif
