#include "common.h"

void run_client(int sockfd, const char *client_id) {
  int rc;
  char buf[MSG_MAXSIZE + 1];
  memset(buf, 0, MSG_MAXSIZE + 1);

  // Structures for sending and receiving packets from/to the server
  struct tcp_packet sent_packet;
  struct udp_to_client_packet recv_packet;

  strcpy(sent_packet.client_id, client_id);
  sent_packet.type = START;
  send_all(sockfd, &sent_packet, sizeof(sent_packet));

  struct pollfd fds[2];  // One pollfd for stdin, one for socket

  // Configure descriptor for stdin
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  // Configure descriptor for socket
  fds[1].fd = sockfd;
  fds[1].events = POLLIN;

  // Multiplex between stdin and receiving messages from the server
  while (1) {
    rc = poll(fds, 2, -1);
    DIE(rc < 0, "poll");

    // Check if there are data available for reading from stdin
    if (fds[0].revents & POLLIN) {
      // Read input from stdin
      if (!fgets(buf, sizeof(buf), stdin)) {
        break;  // Exit if there's an error or EOF
      }

      // Check and process the command
      if (strncmp(buf, "subscribe", 9) == 0) {
        // Subscribe command
        sent_packet.type = SUBSCRIBE;
        strncpy(sent_packet.topic, buf + 10, MAX_TOPIC_LEN);
        sent_packet.topic[strlen(sent_packet.topic) - 1] = '\0';

        rc = send_all(sockfd, &sent_packet, sizeof(sent_packet));
        DIE(rc < 0, "send_all");

        printf("Subscribed to topic %s\n", sent_packet.topic);
      } else if (strncmp(buf, "unsubscribe", 11) == 0) {
        // Unsubscribe command
        sent_packet.type = UNSUBSCRIBE;
        strncpy(sent_packet.topic, buf + 12, MAX_TOPIC_LEN);
        sent_packet.topic[strlen(sent_packet.topic) - 1] = '\0';

        rc = send_all(sockfd, &sent_packet, sizeof(sent_packet));
        DIE(rc < 0, "send_all");

        printf("Unsubscribed from topic %s\n", sent_packet.topic);
      } else if (strncmp(buf, "exit", 4) == 0) {
        // Exit command
        sent_packet.type = EXIT;
        rc = send_all(sockfd, &sent_packet, sizeof(sent_packet));
        DIE(rc < 0, "send_all");
        break;  // Exit the loop
      } else {
        // printf("Unknown command\n");
      }
    }

    // Check if there are data available for reading from the socket
    if (fds[1].revents & POLLIN) {
      // Receive message from the server
      rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
      if (rc <= 0) {
        // Exit if no more data can be received from the server
        break;
      }

      char data_type_string[15];

      switch (recv_packet.data_type) {
        case 0:  // INT
        {
          strcpy(data_type_string, "INT");
          int8_t sign = 0;
          uint32_t num = 0;
          memcpy(&sign, recv_packet.message, sizeof(int8_t));
          memcpy(&num, recv_packet.message + sizeof(int8_t), sizeof(uint32_t));
          num = ntohl(num);  // Convert to host byte order

          // Handle negative numbers
          if (sign == 1) {
            num = -num;
          }

          printf("%s:%d - %s - %s - %d\n", inet_ntoa(recv_packet.sin_addr),
                 ntohs(recv_packet.sin_port), recv_packet.topic,
                 data_type_string, num);
        } break;
        case 1:  // SHORT REAL
        {
          strcpy(data_type_string, "SHORT_REAL");
          uint16_t num = 0;
          memcpy(&num, recv_packet.message, sizeof(uint16_t));
          num = ntohs(num);  // Convert to host byte order
          double real = num / 100.0;

          printf("%s:%d - %s - %s - %.2f\n", inet_ntoa(recv_packet.sin_addr),
                 ntohs(recv_packet.sin_port), recv_packet.topic,
                 data_type_string, real);
        } break;
        case 2:  // FLOAT
        {
          strcpy(data_type_string, "FLOAT");
          int8_t sign = 0;
          uint32_t base = 0;
          uint8_t exp = 0;

          memcpy(&sign, recv_packet.message, sizeof(int8_t));
          memcpy(&base, recv_packet.message + sizeof(int8_t), sizeof(uint32_t));
          memcpy(&exp, recv_packet.message + sizeof(int8_t) + sizeof(uint32_t),
                 sizeof(uint8_t));

          base = ntohl(base);  // Convert to host byte order

          double num = base;

          for (int j = 0; j < exp; j++) {
            num /= 10.0;
          }

          // Handle negative numbers
          if (sign == 1) {
            num = -num;
          }

          // Process and display the received message
          printf("%s:%d - %s - %s - %f\n", inet_ntoa(recv_packet.sin_addr),
                 ntohs(recv_packet.sin_port), recv_packet.topic,
                 data_type_string, num);
        } break;
        case 3:  // STRING
        {
          strcpy(data_type_string, "STRING");
          char message[MSG_MAXSIZE + 1];
          memcpy(message, recv_packet.message, MSG_MAXSIZE);
          message[MSG_MAXSIZE] = '\0';  // Ensure null termination

          printf("%s:%d - %s - %s - %s\n", inet_ntoa(recv_packet.sin_addr),
                 ntohs(recv_packet.sin_port), recv_packet.topic,
                 data_type_string, message);
        } break;
        default:
          break;
      }
    }
  }

  // Close the socket connection
  close(sockfd);
}

int main(int argc, char *argv[]) {
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  int sockfd = -1;

  if (argc != 4) {
    // printf("\n Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", argv[0]);
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

  // Disable Nagle algorithm
  int flag = 1;
  if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) <
      0) {
    perror("setsockopt TCP_NODELAY failed");
  }

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
