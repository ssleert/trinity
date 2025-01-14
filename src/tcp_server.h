#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <netinet/in.h> // for sockaddr_in
#include <stdio.h> // for perror
#include <stdlib.h> // for exit
#include <sys/socket.h> // for socket
#include <unistd.h> // for close

// TCP server configuration structure
typedef struct {
    int port;
    int server_socket;
    struct sockaddr_in server_addr;
    void (*handler)(int client_socket); // User-defined handler callback function
} TCPServer;

/**
 * @brief Initialize the TCP server.
 *
 * @param server The server configuration to initialize.
 * @param port The port number to bind the server to.
 * @param handler A user-defined function to handle client connections.
 * @return int 0 on success, -1 on failure.
 */
int tcp_server_init(TCPServer* server, int port, void (*handler)(int client_socket));

/**
 * @brief Start listening for incoming client connections.
 *
 * @param server The server configuration.
 * @param backlog The maximum number of pending connections in the queue.
 * @return int 0 on success, -1 on failure.
 */
int tcp_server_listen(TCPServer* server, int backlog);

/**
 * @brief Accept a client connection.
 *
 * @param server The server configuration.
 * @param client_socket The client socket that will be filled on success.
 * @return int 0 on success, -1 on failure.
 */
int tcp_server_accept(TCPServer* server, int* client_socket);

/**
 * @brief Close the server and release resources.
 *
 * @param server The server configuration.
 * @return int 0 on success, -1 on failure.
 */
int tcp_server_shutdown(TCPServer* server);

#endif // TCP_SERVER_H
