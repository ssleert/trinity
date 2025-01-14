#include "trinity.h"
#include "db.h"
#include "event_bus.h"
#include "http.h"
#include "log.h"
#include "routes.h"
#include "sqlite3.h"
#include "tcp_server.h"
#include "utils.h"
#include "uuid4.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

// Custom handler function to handle client connections
void client_handler(int client_socket)
{
    LogTrace("client handler called");

    HttpRequest request = { 0 };
    if (http_request_read_from_socket(client_socket, &request) < 0) {
        perror("Http read request failed");
        send(client_socket, HTTP_BAD_REQUEST_RESPONSE, sizeof(HTTP_BAD_REQUEST_RESPONSE) - 1, 0);
        close(client_socket);
        return;
    }

    LogTrace("%s %s", request.method, request.path);
    for (size_t i = 0; i < request.headers_len; ++i) {
        LogTrace("%s", request.headers[i]);
    }
    LogTrace("body_len = %lu '%.*s'", request.body_len, (int)request.body_len, request.body);

    HttpResponse response = { 0 };

    int event_stream_requested = strcmp(request.path, "/events/subscribe") == 0;
    LogTrace("event_stream_requested = %d", event_stream_requested);
    if (event_stream_requested) {
        LogTrace("event stream request");
        pthread_t thrd;

        HttpRequest* req_copy = malloc(sizeof(HttpRequest));
        if (!req_copy) {
            free_http_request(&request);
            return;
        }

        if (copy_http_request(&request, req_copy)) {
            free_http_request(&request);
            free(req_copy);
            return;
        }

        RequestAndResponse* req_and_res = malloc(sizeof(RequestAndResponse));
        req_and_res->req = req_copy;
        req_and_res->res = NULL;

        pthread_create(&thrd, NULL, exec_route_by_path_in_thread, req_and_res);
        pthread_detach(thrd);
        return;
    }

    LogTrace("try to execute route by path");
    int rc = exec_route_by_path(&request, &response);
    if (rc == -255) {
        perror("Not Found Error");
        send(client_socket, HTTP_NOT_FOUND_ERROR, sizeof(HTTP_NOT_FOUND_ERROR) - 1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    }
    if (rc < 0) {
        perror("Internal Server Error");
        send(client_socket, HTTP_INTERNAL_SERVER_ERROR, sizeof(HTTP_INTERNAL_SERVER_ERROR) - 1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    }

    if (http_response_write_to_socket(client_socket, &response)) {
        perror("Http write response failed");
        send(client_socket, HTTP_INTERNAL_SERVER_ERROR, sizeof(HTTP_INTERNAL_SERVER_ERROR) - 1, 0);
        close(client_socket);
        free_http_request(&request);
        return;
    };

    // Close the client socket
    close(client_socket);

    free_http_request(&request);
    free_http_response(&response);
}

void* tcp_worker(void* data)
{
    TCPServer* server = (TCPServer*)data;
    while (true) {
        int client_socket;
        if (tcp_server_accept(server, &client_socket) != 0) {
            fprintf(stderr, "Failed to accept client.\n");
            continue;
        }
    }
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    if (init_global_event_bus()) {
        perror("Cant init global event bus");
    }

    if (sqlite3_initialize()) {
        perror("Error with sqlite3 init");
        return -1;
    }

    if (uuid4_init()) {
        perror("Error With Uuid4 init");
        return -1;
    };

    if (init_db()) {
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

    LogTrace("all threads created");

    // Wait for all threads to complete
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    LogTrace("all threads created");

    return 0;
}
