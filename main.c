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

// Достаточно внятный гайд по поднятию tcp-сервера
// https://devtails.xyz/@adam/how-to-build-a-simple-tcp-server-in-c

int main(void) {
  struct sockaddr_in server_sockaddr_in;
  server_sockaddr_in.sin_family = AF_INET; // internet work
  server_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);

  const int PORT = 3030;
  server_sockaddr_in.sin_port = htons(PORT);

  // creates an endpoint for communication and returns a file descriptor that
  // refers to that endpoint SOCK_STREAM defines that this should communicate
  // over TCP
  int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

  int bindres =
      bind(socket_file_descriptor, (struct sockaddr *)&server_sockaddr_in,
           sizeof(server_sockaddr_in));
  if (bindres == -1) {
    fprintf(stderr, "bind failed: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  listen(socket_file_descriptor, 5);

  printf("listening at %d\n", PORT);

  struct sockaddr_in client_sockaddr_in;

  while (1) {
    socklen_t len = sizeof(client_sockaddr_in);

    int conn_file_descriptor = accept(
        socket_file_descriptor, (struct sockaddr *)&client_sockaddr_in, &len);

    if (conn_file_descriptor == -1) {
      perror("accept failed\n");
      continue;
    }

    char buf[MAX_MSG_SIZE] = {};
    ssize_t bytes_read = read(conn_file_descriptor, buf, sizeof(buf) - 1);

    if (bytes_read > 0) {
      buf[bytes_read] = '\0';

      HttpRequest req;
      if (parse_http_request(buf, &req) == -1) {
        // send 500 TODO
        perror("failed to parse http data\n");
        free_http_request(&req);
        continue;
      }

      printf("%s %s\n", req.method, req.path);

      HttpResponse res;
      memset(&res, 0, sizeof(HttpResponse));
      res.status_code = 200;
      res.status_text = "OK";

      add_header(&res, "Content-Type", "text/plain");
      add_header(&res, "Connection", "close");

      const char *response_str = "Recieved\n";
      res.body = strdup(response_str);
      if (res.body) {
        res.body_len = strlen(res.body);
      }

      char content_length_header[20];
      snprintf(content_length_header, sizeof(content_length_header), "%zu",
               res.body_len);
      add_header(&res, "Content-Length", content_length_header);

      char response_buf[4096];
      int response_size =
          build_http_response(&res, response_buf, sizeof(response_buf));

      int bytes_written =
          write(conn_file_descriptor, response_buf, response_size);
      if (bytes_written == -1) {
        perror("write failed");
      }

      free_http_request(&req);
      free_http_response(&res);
    } else if (bytes_read == 0) {
      printf("client disconnected\n");
    } else {
      fprintf(stderr, "bind failed: %s\n", strerror(errno));
    }

    usleep(1000);
    shutdown(conn_file_descriptor, SHUT_RDWR);
    close(conn_file_descriptor);
  }

  close(socket_file_descriptor);
  return 0;
}
