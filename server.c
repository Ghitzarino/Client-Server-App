#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

#define MAX_CONNECTIONS 32

void run_chat_multi_server(int listenfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS];
  int num_clients = 1;
  int rc;

  // Initialize topic subscriptions for clients
  char topics[MAX_CONNECTIONS][MAX_TOPIC_LEN + 1];
  memset(topics, 0, sizeof(topics));

  struct chat_packet received_packet;

  // Set up socket listenfd for listening
  rc = listen(listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // Add the new file descriptor (listening socket) to the set
  poll_fds[0].fd = listenfd;
  poll_fds[0].events = POLLIN;

  while (1) {
    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_clients; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == listenfd) {
          // A connection request has arrived on the listening socket
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd =
              accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          // Add the new socket returned by accept() to the set of file
          // descriptors
          poll_fds[num_clients].fd = newsockfd;
          poll_fds[num_clients].events = POLLIN;
          num_clients++;

          // Get the client id
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          printf("New client %s connected from %s:%d.\n",
                 received_packet.client_id, inet_ntoa(cli_addr.sin_addr),
                 ntohs(cli_addr.sin_port));
        } else {
          // Data is available to be read on one of the client sockets
          int rc = recv_all(poll_fds[i].fd, &received_packet,
                            sizeof(received_packet));
          DIE(rc < 0, "recv");

          if (rc == 0) {
            // The connection has been closed by the client
            printf("Client %d disconnected\n", poll_fds[i].fd);
            close(poll_fds[i].fd);

            // Remove the closed socket from the set
            for (int j = i; j < num_clients - 1; j++) {
              poll_fds[j] = poll_fds[j + 1];
            }

            num_clients--;
          } else {
            // Process the received packet based on its type
            switch (received_packet.type) {
              case SUBSCRIBE:
                strcpy(topics[i], received_packet.topic);
                printf("Client %d subscribed to topic: %s\n", poll_fds[i].fd,
                       received_packet.topic);
                break;
              case UNSUBSCRIBE:
                topics[i][0] = '\0';  // Unsubscribe by clearing the topic
                printf("Client %d unsubscribed from topic: %s\n",
                       poll_fds[i].fd, received_packet.topic);
                break;
              case EXIT:
                printf("Client %d requested to exit\n", poll_fds[i].fd);
                close(poll_fds[i].fd);

                // Remove the closed socket from the set
                for (int j = i; j < num_clients - 1; j++) {
                  poll_fds[j] = poll_fds[j + 1];
                  strcpy(topics[j], topics[j + 1]);
                }

                num_clients--;
                break;
              case MESSAGE:
                // Broadcast the message to clients subscribed to the topic
                printf("Received message from client %d on topic %s: %s\n",
                       poll_fds[i].fd, received_packet.topic,
                       received_packet.message);
                for (int j = 0; j < num_clients; j++) {
                  if (poll_fds[j].fd != listenfd &&
                      strcmp(topics[j], received_packet.topic) == 0) {
                    // Send the message to clients subscribed to the same topic
                    rc = send_all(poll_fds[j].fd, &received_packet,
                                  sizeof(received_packet));
                    DIE(rc <= 0, "send_all");
                  }
                }
                break;
              default:
                printf("Unknown packet type received from client %d\n",
                       poll_fds[i].fd);
                break;
            }
          }
        }
      }
    }
  }
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (argc != 2) {
    printf("\n Usage: %s <port>\n", argv[0]);
    return 1;
  }

  // Parse the port as a number
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtain a TCP socket for receiving connections
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(listenfd < 0, "socket");

  // Set up server address structure
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  // Make the socket address reusable to avoid errors if the server is run
  // quickly multiple times
  int enable = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  // Bind the server address to the socket
  rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "bind");

  run_chat_multi_server(listenfd);

  // Close listenfd
  close(listenfd);

  return 0;
}
