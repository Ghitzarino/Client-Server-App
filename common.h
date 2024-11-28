#ifndef __COMMON_H__
#define __COMMON_H__

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define DIE(assertion, call_description)                 \
  do {                                                   \
    if (assertion) {                                     \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
      perror(call_description);                          \
      exit(EXIT_FAILURE);                                \
    }                                                    \
  } while (0)

int send_all(int sockfd, void *buff, size_t len);
int recv_all(int sockfd, void *buff, size_t len);

#define MAX_CONNECTIONS 64
#define ID_SIZE 10
#define MAX_TOPIC_LEN 51
#define MSG_MAXSIZE 1500

typedef enum { START, SUBSCRIBE, UNSUBSCRIBE, EXIT } TCP_type;

struct tcp_packet {
  char client_id[ID_SIZE];
  TCP_type type;
  char topic[MAX_TOPIC_LEN];
};

struct udp_to_server_packet {
  char topic[MAX_TOPIC_LEN - 1];
  char data_type;  // 0 - int, 1 - short real, 2 - float, 3 - string
  char message[MSG_MAXSIZE];
};

struct udp_to_client_packet {
  char topic[MAX_TOPIC_LEN - 1];
  char data_type;  // 0 - int, 1 - short real, 2 - float, 3 - string
  char message[MSG_MAXSIZE];
  in_port_t sin_port;       // Port number
  struct in_addr sin_addr;  // Internet address
};

struct client_topics {
  char client_id[ID_SIZE];
  int size, active, client_fd;
  char topics[MAX_CONNECTIONS][MAX_TOPIC_LEN];
};

#endif
