#include "websockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// opcode frames:
// 0x1 - текстовые сообщения  1
// 0x2 - бинарные данные      10
// 0x8 - закрытие соединения  1000
// 0x9 - ping                 1001
// 0xA - pong                 1010

int read_ws_frame(int conn_fd, WSFrame *frame) {
  // заголовок - первые два байта фрейма
  // первый байт содержит флаги и тип (opcode)
  // второй байт - mask и начальную информацию о длине
  uint8_t header[2];
  ssize_t bytes_read = read(conn_fd, header, 2);
  if (bytes_read != 2) {
    return -1;
  };

  // bit masking
  frame->fin = header[0] & 0x80;         // 10000000
  frame->opcode = header[0] & 0x0F;      // 00001111
  frame->mask = header[1] & 0x80;        // 10000000
  frame->payload_len = header[1] & 0x7F; // 01111111

  size_t offset = 2; // read bytes counter

  // payload_len - изначальная информация о длине сообщения
  // если <= 125 - то это и есть реальная длина
  // 126 - реальная длина в следующих 2 байтах
  // 127 - реальная длина в следующих 8 байтах
  if (frame->payload_len == 126) {
    uint8_t len_buf[2];
    if (read(conn_fd, len_buf, 2) != 2) {
      printf("invalid payload_len\n");
      return -1;
    };

    frame->payload_len = (len_buf[0] << 8) | len_buf[1];
    offset += 2;
  } else if (frame->payload_len == 127) {
    uint8_t len_buf[8];
    if (read(conn_fd, len_buf, 8) != 8) {
      printf("invalid payload_len\n");
      return -1;
    }

    frame->payload_len = 0;
    for (int i = 0; i < 8; i++) {
      frame->payload_len = (frame->payload_len << 8) | len_buf[i];
    }

    offset += 8;
  }

  if (frame->mask) {
    if (read(conn_fd, frame->masking_key, 4) != 4) {
      return -1;
    }

    offset += 4;
  }

  frame->payload = malloc(frame->payload_len);
  if (read(conn_fd, frame->payload, frame->payload_len) != frame->payload_len) {
    free(frame->payload);
    return -1;
  }

  if (frame->mask) {
    for (size_t i = 0; i < frame->payload_len; i++) {
      frame->payload[i] ^= frame->masking_key[i % 4];
    }
  }

  return offset + frame->payload_len;
}

int send_ws_frame(int conn_fd, const uint8_t *payload, size_t payload_len,
                  uint8_t opcode) {
  uint8_t header[14]; // max header size
  size_t header_len = 0;

  header[header_len++] = 0x80 | opcode;

  if (payload_len <= 125) {
    header[header_len++] = payload_len;
  } else if (payload_len <= 65535) {
    header[header_len++] = 126;
    header[header_len++] = (payload_len >> 8) & 0xFF;
    header[header_len++] = payload_len & 0xFF;
  } else {
    header[header_len++] = 127;
    for (int i = 7; i >= 0; i--) {
      header[header_len++] = (payload_len >> (8 * i)) & 0xFF;
    }
  }

  // write header
  if (write(conn_fd, header, header_len) != header_len)
    return -1;
  if (write(conn_fd, payload, payload_len) != payload_len)
    return -1;

  return header_len + payload_len;
}

void handle_ws_connection(int conn_fd) {
  while (1) {
    WSFrame frame;
    int bytes_read = read_ws_frame(conn_fd, &frame);
    if (bytes_read <= 0) {
      printf("0 bytes read. Closing connection\n");
      break;
    }

    int frame_code = handle_ws_frame(conn_fd, &frame);
    if (frame_code == -1) {
      break;
    }

    free(frame.payload);
  }

  close(conn_fd);
}

int handle_ws_frame(int conn_fd, WSFrame *frame) {
  printf("opcode: 0x%02X\n", frame->opcode);

  switch (frame->opcode) {
  case 0x1:
    printf("received text: %.*s\n", (int)frame->payload_len, frame->payload);
    send_ws_frame(conn_fd, frame->payload, frame->payload_len, frame->opcode);
    break;
  case 0x2:
    printf("recieved binary: %lu\n", frame->payload_len);
    break;
  case 0x8:
    send_ws_frame(conn_fd, NULL, 0, 0x8);
    break;
  case 0x9:
    send_ws_frame(conn_fd, frame->payload, frame->payload_len, 0xA);
    break;
  case 0xA:
    break;
  default:
    printf("Unknown opcode: 0x%02X\n", frame->opcode);
    break;
  }

  return 0;
}
