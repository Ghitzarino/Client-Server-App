#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining > 0) {
    int bytes = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
    if (bytes <= 0) {
      // Eroare sau conexiune închisă
      return bytes;
    }
    bytes_received += bytes;
    bytes_remaining -= bytes;
  }

  return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining > 0) {
    int bytes = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
    if (bytes <= 0) {
      // Eroare sau conexiune închisă
      return bytes;
    }
    bytes_sent += bytes;
    bytes_remaining -= bytes;
  }

  return bytes_sent;
}
