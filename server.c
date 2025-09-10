#include "server.h"
#include "http.h"
#include "websockets.h"

int start_http_server(int port) {
  struct sockaddr_in server_sockaddr_in;
  server_sockaddr_in.sin_family = AF_INET; // internet work
  server_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);

  server_sockaddr_in.sin_port = htons(port);

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

  listen(socket_file_descriptor, LISTEN_BACKLOG);

  printf("listening at %d\n", port);

  struct sockaddr_in client_sockaddr_in;

  while (1) {
    socklen_t len = sizeof(client_sockaddr_in);

    int conn_file_descriptor = accept(
        socket_file_descriptor, (struct sockaddr *)&client_sockaddr_in, &len);

    if (conn_file_descriptor == -1) {
      perror("accept failed\n");
      continue;
    }

    handle_request(conn_file_descriptor);
  }

  close(socket_file_descriptor);
  return 0;
}

int handle_request(int conn_fd) {
  char buf[MAX_MSG_SIZE] = {};
  ssize_t bytes_read = read(conn_fd, buf, sizeof(buf) - 1);

  if (bytes_read > 0) {
    buf[bytes_read] = '\0';

    HttpRequest req;
    int parse_req_result = parse_http_request(buf, &req);
    if (parse_req_result == -1) {
      HttpResponse res;
      create_err_response(&res, 500, "Internal Server Error",
                          "Failed to read request");

      char response_buf[4096];
      int response_size =
          build_http_response(&res, response_buf, sizeof(response_buf));

      int bytes_written = write(conn_fd, response_buf, response_size);
      if (bytes_written == -1) {
        perror("write failed");
      }

      free_http_request(&req);
      free_http_response(&res);
      return 1;
    }

    if (is_websocket_handshake(&req)) {
        printf("Websocket handshake!\n");
    }

    printf("%s %s\n", req.method, req.path);

    HttpResponse res;
    create_ok_response(&res);

    char response_buf[4096];
    int response_size =
        build_http_response(&res, response_buf, sizeof(response_buf));

    int bytes_written = write(conn_fd, response_buf, response_size);
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
  shutdown(conn_fd, SHUT_RDWR);
  close(conn_fd);
  return 0;
}
