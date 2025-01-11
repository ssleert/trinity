#include "tcp_server.h"
#include "http.h"
#include "routes.h"
#include "uuid4.h"
#include "db.h"
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

// Custom handler function to handle client connections
void client_handler(int client_socket) {
    HttpRequest request = {0};
    if (http_request_read_from_socket(client_socket, &request) < 0) {
        perror("Http read request failed");
        send(client_socket, HTTP_BAD_REQUEST_RESPONSE, sizeof(HTTP_BAD_REQUEST_RESPONSE)-1, 0);
        close(client_socket);
        return;
    }
    
    HttpResponse response = {0};

    int rc = exec_route_by_path(&request, &response);
    if (rc == -255) {
        perror("Not Found Error");
        send(client_socket, HTTP_NOT_FOUND_ERROR, sizeof(HTTP_NOT_FOUND_ERROR)-1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    }
    if (rc < 0) { 
        perror("Internal Server Error");
        send(client_socket, HTTP_INTERNAL_SERVER_ERROR, sizeof(HTTP_INTERNAL_SERVER_ERROR)-1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    }
    
    if (http_response_write_to_socket(client_socket, &response)) {
        perror("Http write response failed");
        send(client_socket, HTTP_INTERNAL_SERVER_ERROR, sizeof(HTTP_INTERNAL_SERVER_ERROR)-1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    };

    // Close the client socket
    close(client_socket);

    free_http_request(&request);
    free_http_response(&response);
}

void* tcp_worker(void* data) {
    TCPServer* server = (TCPServer*)data;
    while (true) {
        int client_socket;
        if (tcp_server_accept(server, &client_socket) != 0) {
            fprintf(stderr, "Failed to accept client.\n");
            continue;
        }
    }
}

#define PORT 8020
#define NUM_THREADS 20

int main(void) {
    if (uuid4_init()) {
        perror("Error With Uuid4 init");
        return -1;
    };
    
    if (init_db("main.db")) {
        perror("Error with db");
        return -1;
    }

    TCPServer server;

    // Initialize the server with the user-defined handler
    if (tcp_server_init(&server, PORT, client_handler) != 0) {
        fprintf(stderr, "Server initialization failed.\n");
        return -1;
    }

    // Start listening for client connections
    if (tcp_server_listen(&server, 5) != 0) {
        fprintf(stderr, "Server failed to listen.\n");
        return -1;
    }
    printf("Server is listening on port %d...\n", PORT);

    pthread_t threads[NUM_THREADS];
    int rc;
    long t;
    for (t = 0; t < NUM_THREADS; t++) {
        printf("Main: creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, &tcp_worker, &server);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    // Wait for all threads to complete
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    return 0;
}
