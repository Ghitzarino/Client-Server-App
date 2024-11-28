TCP and UDP Client-Server Application

This project was a challenging yet rewarding experience, taking approximately 
25-30 hours over the course of a week to fully implement the functionality. 
Here's an overview of the application and its features:


Application Overview

The project is a client-server system enabling users to subscribe and unsubscribe
to various topics to receive messages. The server facilitates this interaction
using both TCP and UDP connections:
-TCP: Manages client-server communication, including subscriptions.
-UDP: Used for efficient message delivery to subscribed clients.


Key Features:

1) Initial Connection
When a client connects to the server, it sends a START packet containing a unique
client ID. The server uses this ID to register and track the connection.

2) Subscription Management
Clients can subscribe to a topic by sending a SUBSCRIBE packet with their ID and the topic name.
The server maintains a mapping of clients and their subscribed topics to facilitate message delivery.
Unsubscriptions are handled similarly, ensuring the topic list for each client stays up to date.

3) Message Transmission
Messages sent to a topic are relayed by the server to all subscribed clients.
Messages are structured in UDP_TO_CLIENT_PACKET format, containing details such as the topic, 
message type, the message itself, and the sender's IP and port.


Implementation Details:

-Data Integrity: The code ensures proper handling of all operations to avoid data loss and 
communication issues.
-Wildcard Topics: Topics support wildcard patterns for flexible subscriptions.
-Unique Client IDs: Duplicate IDs are not allowed. If a client attempts to connect with an existing 
ID, the connection is automatically terminated.
-Server Shutdown: When the server shuts down, all connected clients are gracefully disconnected.
-Efficient Networking: The Nagle algorithm is disabled for improved real-time communication.
-Resource Management: Memory allocation for file descriptors and topic lists is static, with a limit
on the number of concurrent clients.
-Byte Order: Network byte order conversions are correctly handled throughout.

This project showcases a custom application-layer protocol designed for efficient and reliable
communication between clients and the server. It was a rewarding experience, improving my understanding
of networking and system design.