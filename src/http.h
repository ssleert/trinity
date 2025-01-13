#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define HTTP_BAD_REQUEST_RESPONSE "HTTP/1.0 400\nAsd: bsd\nContent-Length: 2\n\nok"
#define HTTP_INTERNAL_SERVER_ERROR "HTTP/1.0 500\n\n500 Internal Server Error"
#define HTTP_NOT_FOUND_ERROR "HTTP/1.0 404\n\n404 Not Found"

typedef struct {
    int socket;
    char method[16];
    char path[256];
    size_t headers_len;
    char** headers;
    size_t body_len;
    char* body;
} HttpRequest;

typedef struct {
    char* status;
    size_t headers_len;
    char** headers; 
    size_t body_len;
    char* body;
} HttpResponse;

int http_request_read_from_socket(int socket, HttpRequest* http_request);
void free_http_request(HttpRequest* http_request);
int copy_http_request(HttpRequest* first, HttpRequest* second);

int http_response_write_to_socket(int socket, HttpResponse* http_response);

HttpResponse* create_http_response(HttpResponse* response, const char* status, const char** headers, size_t headers_count, const char* body);
void free_http_response(HttpResponse* response);
int copy_http_response(HttpResponse* first, HttpResponse* second);

#endif
