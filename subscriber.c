#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

void run_client(int sockfd, const char *client_id) {
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  struct chat_packet sent_packet;
  strcpy(sent_packet.client_id, client_id);
  sent_packet.type = START;
  send_all(sockfd, &sent_packet, sizeof(sent_packet));

  struct chat_packet recv_packet;

  struct pollfd fds[2];  // One pollfd for stdin, one for socket
  int timeout = -1;      // Wait indefinitely until an event occurs

  // Configure descriptor for stdin
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  // Configure descriptor for socket
  fds[1].fd = sockfd;
  fds[1].events = POLLIN;

  // Multiplex between reading from stdin and receiving messages from the server
  while (1) {
    int ret = poll(fds, 2, timeout);
    if (ret < 0) {
      perror("poll");
      break;
    }

    // Check if there are data available for reading from stdin
    if (fds[0].revents & POLLIN) {
      // Read from stdin
      if (!fgets(buf, sizeof(buf), stdin) || isspace(buf[0])) {
        break;  // Exit if an empty line or EOF is entered
      }

      // Check and process the command
      if (strncmp(buf, "subscribe", 9) == 0) {
        // Subscribe command
        sent_packet.type = SUBSCRIBE;
        strncpy(sent_packet.topic, buf + 10, MAX_TOPIC_LEN);
        sent_packet.topic[strlen(sent_packet.topic) - 1] =
            '\0';  // Remove newline character
        send_all(sockfd, &sent_packet, sizeof(sent_packet));

        printf("Subscribed to topic %s\n", sent_packet.topic);
      } else if (strncmp(buf, "unsubscribe", 11) == 0) {
        // Unsubscribe command
        sent_packet.type = UNSUBSCRIBE;
        strncpy(sent_packet.topic, buf + 12, MAX_TOPIC_LEN);
        sent_packet.topic[strlen(sent_packet.topic) - 1] =
            '\0';  // Remove newline character
        send_all(sockfd, &sent_packet, sizeof(sent_packet));

        printf("Unsubscribed from topic %s\n", sent_packet.topic);
      } else if (strncmp(buf, "exit", 4) == 0) {
        // Exit command
        sent_packet.type = EXIT;
        send_all(sockfd, &sent_packet, sizeof(sent_packet));
        break;  // Exit the loop
      }
    }

    // Check if there are data available for reading from the socket
    if (fds[1].revents & POLLIN) {
      // Receive message from the server
      int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
      if (rc <= 0) {
        break;  // Exit if no more data can be received from the server
      }

      // Process and display the received message
      printf("%s\n", recv_packet.message);
    }
  }

  // Close the socket connection
  close(sockfd);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd = -1;

  if (argc != 4) {
    printf("\n Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
    return 1;
  }

  const char *client_id = argv[1];

  // Parse the server IP and port number
  char *ip_server = argv[2];
  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Create TCP socket for connecting to the server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Set up server address structure
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);
  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, ip_server, &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Connect to the server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  run_client(sockfd, client_id);

  return 0;
}
