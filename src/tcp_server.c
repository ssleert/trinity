#include "tcp_server.h"

// Initialize the server by setting up the socket, binding it to the specified port, and setting the callback handler
int tcp_server_init(TCPServer* server, int port, void (*handler)(int client_socket))
{
    server->port = port;
    server->handler = handler;

    // Create a TCP socket
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
        perror("Socket creation failed");
        return -1;
    }

    if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &(int) { 1 }, sizeof(int)) < 0) {
        perror("Set socket reuse failed");
        return -1;
    }

    // Set the server address structure
    server->server_addr.sin_family = AF_INET;
    server->server_addr.sin_addr.s_addr = INADDR_ANY;
    server->server_addr.sin_port = htons(port);

    // Bind the socket to the address and port
    if (bind(server->server_socket, (struct sockaddr*)&server->server_addr, sizeof(server->server_addr)) < 0) {
        perror("Bind failed");
        close(server->server_socket);
        return -1;
    }

    return 0;
}

// Start listening for incoming client connections
int tcp_server_listen(TCPServer* server, int backlog)
{
    if (listen(server->server_socket, backlog) < 0) {
        perror("Listen failed");
        return -1;
    }
    return 0;
}

// Accept a client connection and call the user-defined handler
int tcp_server_accept(TCPServer* server, int* client_socket)
{
    socklen_t addr_len = sizeof(server->server_addr);
    *client_socket = accept(server->server_socket, (struct sockaddr*)&server->server_addr, &addr_len);
    if (*client_socket < 0) {
        perror("Accept failed");
        return -1;
    }

    // Call the user-defined handler (callback) for the client connection
    if (server->handler != NULL) {
        server->handler(*client_socket);
    }

    return 0;
}

// Shutdown the server and close the socket
int tcp_server_shutdown(TCPServer* server)
{
    if (close(server->server_socket) < 0) {
        perror("Server shutdown failed");
        return -1;
    }
    return 0;
}
