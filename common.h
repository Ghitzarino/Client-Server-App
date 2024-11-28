#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

#define ID_SIZE 10
#define MAX_TOPIC_LEN 50
#define MSG_MAXSIZE 1500

typedef enum { START, SUBSCRIBE, UNSUBSCRIBE, EXIT, MESSAGE } PacketType;

struct chat_packet {
  char client_id[ID_SIZE];
  PacketType type;            // Type of the packet
  char topic[MAX_TOPIC_LEN];  // Topic string (null-terminated)
  size_t len;                 // Length of the message content
  char message[MSG_MAXSIZE];  // Message content
};

#endif
