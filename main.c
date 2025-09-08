#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MSG_SIZE 256

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

  listen(socket_file_descriptor, 1);

  printf("waiting...\n");

  struct sockaddr_in client_sockaddr_in;

  // new
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
      printf("recieved %s\n", buf);

      char status = 0;

      write(conn_file_descriptor, &status, 1);
    } else if (bytes_read == 0) {
      printf("client disconnected\n");
    } else {
      perror("read failed\n");
    }

    close(conn_file_descriptor);
  }

  close(socket_file_descriptor);
  return 0;
}
