#include "common.h"

int match_topic(const char *topic, const char *pattern) {
  while (*topic && *pattern) {
    if (*pattern == '+') {
      // Match one level
      while (*topic && *topic != '/') {
        topic++;
      }
      pattern++;
    } else if (*pattern == '*') {
      pattern++;
      // If pattern has reached the end, match all remaining levels
      if (*pattern == '\0') {
        return 1;
      }
      pattern++;

      // Find the next occurrence of the word following '*'
      const char *next_pattern_word = pattern;

      // Move pattern to the next '/'
      while (*pattern && *pattern != '/') {
        pattern++;
      }

      // Search for the matching topic
      while (*topic) {
        if (strncmp(topic, next_pattern_word, pattern - next_pattern_word) ==
                0 &&
            (*(topic + (pattern - next_pattern_word)) == '/' ||
             *(topic + (pattern - next_pattern_word)) == '\0')) {
          break;
        }
        topic++;
      }
      // If no match found, return false
      if (*topic == '\0') {
        return 0;
      }
      // Move topic to the end of the matched word
      topic += pattern - next_pattern_word;
    } else if (*topic != *pattern) {
      return 0;
    } else {
      topic++;
      pattern++;
    }
  }

  return *topic == '\0' && *pattern == '\0';
}

void run_chat_multi_server(int tcp_listenfd, int udp_listenfd) {
  struct pollfd poll_fds[MAX_CONNECTIONS];
  memset(poll_fds, 0, sizeof(poll_fds));

  int num_clients = 0, num_topics = 0;
  int rc;

  // Structures for sending/receiving packets to/from TCP/UDP
  struct tcp_packet recv_tcp_packet;
  struct udp_to_server_packet recv_udp_packet;
  struct udp_to_client_packet sent_udp_packet;
  struct client_topics topics[MAX_CONNECTIONS];
  memset(topics, 0, sizeof(topics));

  // Set up TCP socket for listening
  rc = listen(tcp_listenfd, MAX_CONNECTIONS);
  DIE(rc < 0, "listen");

  // Configure descriptor for stdin
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;

  // Add TCP socket to the polling set
  poll_fds[1].fd = tcp_listenfd;
  poll_fds[1].events = POLLIN;

  // Add UDP socket to the polling set
  poll_fds[2].fd = udp_listenfd;
  poll_fds[2].events = POLLIN;

  num_clients += 3;  // TCP, UDP and STDIN sockets

  while (1) {
    int end = 0;
    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");

    for (int i = 0; i < num_clients; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == tcp_listenfd) {
          // New TCP client connection
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd =
              accept(tcp_listenfd, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          // Disable Nagle algorithm
          int flag = 1;
          if (setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                         sizeof(int)) < 0) {
            perror("setsockopt TCP_NODELAY failed");
          }

          // Get the client ID
          int rc =
              recv_all(newsockfd, &recv_tcp_packet, sizeof(recv_tcp_packet));
          DIE(rc < 0, "recv_all");

          int ok = 1;

          for (int j = 0; j < num_topics; j++) {
            if (strcmp(topics[j].client_id, recv_tcp_packet.client_id) == 0 &&
                topics[j].active == 1) {
              // Client already connected
              ok = 0;
              close(newsockfd);
              break;
            } else if (strcmp(topics[j].client_id, recv_tcp_packet.client_id) ==
                       0) {
              // Client was connected before
              ok = -1;
              topics[j].active = 1;
              topics[j].client_fd = newsockfd;
              break;
            }
          }

          // New client
          if (ok == 1) {
            // Add the new TCP client socket to the polling set
            poll_fds[num_clients].fd = newsockfd;
            poll_fds[num_clients].events = POLLIN;
            num_clients++;

            strcpy(topics[num_topics].client_id, recv_tcp_packet.client_id);
            topics[num_topics].active = 1;
            topics[num_topics].client_fd = newsockfd;
            num_topics++;

            printf("New client %s connected from %s:%d.\n",
                   recv_tcp_packet.client_id, inet_ntoa(cli_addr.sin_addr),
                   ntohs(cli_addr.sin_port));
          } else if (ok == 0) {
            printf("Client %s already connected.\n", recv_tcp_packet.client_id);
          } else {
            // Update the sock fd for the client
            poll_fds[num_clients].fd = newsockfd;
            poll_fds[num_clients].events = POLLIN;
            num_clients++;

            printf("New client %s connected from %s:%d.\n",
                   recv_tcp_packet.client_id, inet_ntoa(cli_addr.sin_addr),
                   ntohs(cli_addr.sin_port));
          }

        } else if (poll_fds[i].fd == udp_listenfd) {
          // Receive message from UDP client
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          rc = recvfrom(udp_listenfd, &recv_udp_packet, sizeof(recv_udp_packet),
                        0, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(rc < 0, "recvfrom");

          // Build the sending package with ip and port
          sent_udp_packet.data_type = recv_udp_packet.data_type;
          memcpy(sent_udp_packet.message, recv_udp_packet.message, MSG_MAXSIZE);
          strcpy(sent_udp_packet.topic, recv_udp_packet.topic);
          sent_udp_packet.sin_addr = cli_addr.sin_addr;
          sent_udp_packet.sin_port = cli_addr.sin_port;

          // Send the package to the subscriber
          for (int j = 0; j < num_topics; j++) {
            if (topics[j].active) {
              for (int k = 0; k < topics[j].size; k++) {
                if (match_topic(recv_udp_packet.topic, topics[j].topics[k])) {
                  int rc = send_all(topics[j].client_fd, &sent_udp_packet,
                                    sizeof(sent_udp_packet));
                  DIE(rc < 0, "send_all");
                  break;
                }
              }
            }
          }
        } else if (poll_fds[i].fd == STDIN_FILENO) {
          // Check for server exit
          char buf[5];
          if (fgets(buf, sizeof(buf), stdin) != NULL &&
              strncmp(buf, "exit", 4) == 0) {
            // Close all TCP client connections
            for (int i = 3; i < num_clients; i++) {
              if (poll_fds[i].fd != -1) {
                close(poll_fds[i].fd);
                poll_fds[i].fd = -1;
              }
            }

            // Close the server
            end = 1;
            break;
          }
        } else {
          // TCP client message
          int rc = recv_all(poll_fds[i].fd, &recv_tcp_packet,
                            sizeof(recv_tcp_packet));
          DIE(rc < 0, "recv_all");

          // Process the received packet based on its type
          switch (recv_tcp_packet.type) {
            case SUBSCRIBE:
              for (int j = 0; j < num_topics; j++) {
                // Match the client ID
                if (topics[j].active &&
                    strcmp(topics[j].client_id, recv_tcp_packet.client_id) ==
                        0) {
                  int ok = 1;

                  // Check to not be duplicate
                  for (int k = 0; k < topics[j].size; k++) {
                    if (strcmp(topics[j].topics[k], recv_tcp_packet.topic) ==
                        0) {
                      ok = 0;
                      break;
                    }
                  }

                  // Add the new topic to the topics list of the client
                  if (ok) {
                    strcpy(topics[j].topics[topics[j].size],
                           recv_tcp_packet.topic);
                    topics[j].size++;
                  }

                  break;
                }
              }

              break;
            case UNSUBSCRIBE:

              for (int j = 0; j < num_topics; j++) {
                // Match the client ID
                if (topics[j].active &&
                    strcmp(topics[j].client_id, recv_tcp_packet.client_id) ==
                        0) {
                  for (int k = 0; k < topics[j].size; k++) {
                    // Delete the topic
                    if (strcmp(topics[j].topics[k], recv_tcp_packet.topic) ==
                        0) {
                      for (int l = k; l < topics[j].size; l++) {
                        strcpy(topics[j].topics[l], topics[j].topics[l + 1]);
                      }
                      topics[j].size--;
                      break;
                    }
                  }
                  break;
                }
              }

              break;
            case EXIT:
              printf("Client %s disconnected.\n", recv_tcp_packet.client_id);
              close(poll_fds[i].fd);

              // Remove the closed socket from the poll
              for (int j = i; j < num_clients; j++) {
                poll_fds[j] = poll_fds[j + 1];
              }

              // Mark the client topics inactive
              for (int j = 0; j < num_topics; j++) {
                if (strcmp(topics[j].client_id, recv_tcp_packet.client_id) ==
                    0) {
                  topics[j].active = 0;
                }
              }

              num_clients--;
              break;
            default:
              break;
          }
        }
      }
    }

    if (end) {
      break;
    }
  }

  close(tcp_listenfd);
  close(udp_listenfd);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (argc != 2) {
    // printf("\n Usage: %s <PORT>\n", argv[0]);
    return 1;
  }

  // Parse the port number
  uint16_t port;
  int rc = sscanf(argv[1], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Create TCP socket for accepting connections
  int tcp_listenfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcp_listenfd < 0, "socket");

  // Disable Nagle algorithm
  int flag = 1;
  if (setsockopt(tcp_listenfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                 sizeof(int)) < 0) {
    perror("setsockopt TCP_NODELAY failed");
  }

  // Create UDP socket for receiving messages
  int udp_listenfd = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_listenfd < 0, "socket");

  // Enable address reuse for TCP socket
  int enable = 1;
  if (setsockopt(tcp_listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
      0)
    perror("setsockopt(SO_REUSEADDR) failed");

  // Set up server address structure for both TCP and UDP sockets
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);
  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  // Bind TCP socket
  rc = bind(tcp_listenfd, (const struct sockaddr *)&serv_addr, socket_len);
  DIE(rc < 0, "bind");

  // Bind UDP socket
  rc = bind(udp_listenfd, (const struct sockaddr *)&serv_addr, socket_len);
  DIE(rc < 0, "bind");

  // Run the server
  run_chat_multi_server(tcp_listenfd, udp_listenfd);

  return 0;
}
