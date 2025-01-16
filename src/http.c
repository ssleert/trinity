#include "http.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONTENT_LEN "Content-Length: "
#define BUFFER_SIZE 1024

static void free_headers(char** headers, size_t headers_len)
{
    if (headers == NULL) {
        return;
    }
    for (size_t i = 0; i < headers_len; ++i) {
        free(headers[i]);
    }
    free(headers);
}

// Helper function to read a line from the socket
static ssize_t read_line(int socket, char* buffer, size_t size)
{
    ssize_t total_read = 0;
    ssize_t bytes_read;
    char ch;

    while ((size_t)total_read < size - 1) {
        bytes_read = read(socket, &ch, 1);
        if (bytes_read <= 0) {
            return bytes_read;
        }

        buffer[total_read++] = ch;
        if (ch == '\n') {
            break;
        }
    }

    buffer[total_read] = '\0';
    return total_read;
}

static ssize_t read_body(int socket, size_t content_len, char* body)
{
    size_t i = 0;
    char ch = 0;

    for (i = 0; i < content_len; ++i) {
        size_t bytes_read = read(socket, &ch, 1);
        if (bytes_read <= 0) {
            return i;
        }

        body[i] = ch;
    }

    return content_len;
}

// Function to parse HTTP request from socket
int http_request_read_from_socket(int socket, HttpRequest* http_request)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    http_request->socket = socket;

    // Read request line (e.g., "GET /path HTTP/1.1")
    bytes_read = read_line(socket, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        perror("Error reading from socket");
        return -1; // Error or connection closed
    }

    // Parse method and path from the request line
    if (sscanf(buffer, "%15s %255s", http_request->method, http_request->path) != 2) {
        return -1; // Invalid request line format
    }

    // Initialize headers and body
    http_request->headers = malloc(sizeof(char*) * 64); // Allocate space for headers
    if (!http_request->headers) {
        return -1; // Memory allocation failure
    }

    int header_count = 0;
    size_t body_len = 0;

    // Read headers until an empty line (end of headers)
    while (true) {
        bytes_read = read_line(socket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            free_headers(http_request->headers, header_count);
            return -1; // Error reading headers or connection closed
        }

        if (buffer[0] == '\r' || buffer[0] == '\n') {
            break; // End of headers
        }

        // Allocate memory for the header and copy it
        http_request->headers[header_count] = strdup(buffer);
        if (http_request->headers[header_count] == NULL) {
            free_headers(http_request->headers, header_count);
            return -1; // Memory allocation failure
        }

        char* nl_char = strchr(http_request->headers[header_count], '\n');
        char* cr_char = strchr(http_request->headers[header_count], '\r');
        if (cr_char != NULL) {
            *cr_char = '\0';
        }
        if (nl_char != NULL) {
            *nl_char = '\0';
        }

        header_count++;

        char* content_len_ptr = strstr(buffer, CONTENT_LEN);
        if (content_len_ptr != NULL) {
            content_len_ptr += sizeof(CONTENT_LEN) - 1;
            if (sscanf(content_len_ptr, "%12lu", &body_len) != 1) {
                return -1;
            };
        }
    }
    http_request->headers_len = header_count;

    char* body = malloc(body_len);
    if (body == NULL) {
        free_headers(http_request->headers, header_count);
        return -1;
    }

    body_len = read_body(socket, body_len, body);
    if (body_len < 0) {
        free_headers(http_request->headers, header_count);
        return -1;
    }

    http_request->body_len = body_len;
    http_request->body = body;

    return 0; // Successfully parsed the HTTP request
}

void free_http_request(HttpRequest* http_request)
{
    free_headers(http_request->headers, http_request->headers_len);
    free(http_request->body);
}

int copy_http_request(HttpRequest* first, HttpRequest* second)
{
    if (!first || !second) {
        return -1; // Error: Null pointer passed
    }

    // Copy primitive fields
    second->socket = first->socket;
    strncpy(second->method, first->method, sizeof(second->method) - 1);
    second->method[sizeof(second->method) - 1] = '\0';
    strncpy(second->path, first->path, sizeof(second->path) - 1);
    second->path[sizeof(second->path) - 1] = '\0';

    // Copy headers
    second->headers_len = first->headers_len;
    if (first->headers_len > 0) {
        second->headers = (char**)malloc(first->headers_len * sizeof(char*));
        if (!second->headers) {
            return -1; // Error: Memory allocation failed
        }

        for (size_t i = 0; i < first->headers_len; i++) {
            size_t header_len = strlen(first->headers[i]) + 1;
            second->headers[i] = (char*)malloc(header_len);
            if (!second->headers[i]) {
                // Free previously allocated headers
                for (size_t j = 0; j < i; j++) {
                    free(second->headers[j]);
                }
                free(second->headers);
                return -1; // Error: Memory allocation failed
            }
            strncpy(second->headers[i], first->headers[i], header_len);
        }
    } else {
        second->headers = NULL;
    }

    // Copy body
    second->body_len = first->body_len;
    if (first->body_len > 0) {
        second->body = (char*)malloc(first->body_len);
        if (!second->body) {
            // Free headers if body allocation fails
            if (second->headers) {
                for (size_t i = 0; i < second->headers_len; i++) {
                    free(second->headers[i]);
                }
                free(second->headers);
            }
            return -1; // Error: Memory allocation failed
        }
        memcpy(second->body, first->body, first->body_len);
    } else {
        second->body = NULL;
    }

    return 0; // Success
}

/**
 * Adds CORS headers to the HttpResponse to allow all origins.
 * 
 * @param http_response Pointer to the HttpResponse structure.
 * @return 0 on success, -1 on error (e.g., memory allocation failure).
 */
int http_response_add_cors_headers(HttpResponse* http_response) {
    if (http_response == NULL) {
        return -1;
    }


    // Allocate memory for the new headers
    size_t new_headers_len = http_response->headers_len + 3; // Adding 3 CORS headers
    char** new_headers = (char**)realloc(http_response->headers, new_headers_len * sizeof(char*));
    if (new_headers == NULL) {
        return -1; // Memory allocation failed
    }

    http_response->headers = new_headers;

    // Add the new headers
    http_response->headers[http_response->headers_len] = strdup(ALLOW_ORIGIN_HEADER);
    http_response->headers[http_response->headers_len + 1] = strdup(ALLOW_METHODS_HEADER);
    http_response->headers[http_response->headers_len + 2] = strdup(ALLOW_HEADERS_HEADER);

    // Check for allocation failures
    if (!http_response->headers[http_response->headers_len] ||
        !http_response->headers[http_response->headers_len + 1] ||
        !http_response->headers[http_response->headers_len + 2]) {
        // Free partially allocated headers
        free(http_response->headers[http_response->headers_len]);
        free(http_response->headers[http_response->headers_len + 1]);
        free(http_response->headers[http_response->headers_len + 2]);
        return -1;
    }

    // Update the headers length
    http_response->headers_len = new_headers_len;

    return 0;
}

int http_response_write_to_socket(int socket, HttpResponse* http_response)
{
    // Write status line
    char status_line[512];
    snprintf(status_line, sizeof(status_line), "HTTP/1.1 %s\r\n", http_response->status);
    if (write(socket, status_line, strlen(status_line)) < 0) {
        return -1; // Failed to write to socket
    }

    // Write headers
    if (http_response->headers != NULL) {
        for (size_t i = 0; i < http_response->headers_len; ++i) {
            char* header = http_response->headers[i];
            if (write(socket, header, strlen(header)) < 0) {
                return -1; // Failed to write to socket
            }
            if (write(socket, "\r\n", 2) < 0) { // End of header line
                return -1;
            }
        }
    }

    // Write a blank line to separate headers from body
    if (write(socket, "\r\n", 2) < 0) {
        return -1; // Failed to write to socket
    }

    // Write body
    if (http_response->body_len > 0 && http_response->body != NULL) {
        if (write(socket, http_response->body, http_response->body_len) < 0) {
            return -1; // Failed to write to socket
        }
    }

    return 0; // Success
}

// Constructor for HttpResponse
HttpResponse* create_http_response(HttpResponse* response, const char* status, const char** headers, size_t headers_count, const char* body)
{
    // Duplicate the status string
    response->status = strdup(status);
    if (!response->status) {
        free(response);
        return NULL;
    }

    // Allocate memory for headers
    response->headers_len = headers_count;
    if (headers_count > 0) {
        response->headers = (char**)malloc(headers_count * sizeof(char*));
        if (!response->headers) {
            free(response->status);
            free(response);
            return NULL;
        }

        // Copy each header
        for (size_t i = 0; i < headers_count; ++i) {
            response->headers[i] = strdup(headers[i]);
            if (!response->headers[i]) {
                free_headers(response->headers, i); // Free allocated headers up to this point
                free(response->status);
                free(response);
                return NULL;
            }
        }
    } else {
        response->headers = NULL;
    }

    // Duplicate the body
    if (body) {
        response->body_len = strlen(body);
        response->body = (char*)malloc(response->body_len + 1);
        if (!response->body) {
            free_headers(response->headers, headers_count);
            free(response->status);
            free(response);
            return NULL;
        }
        strcpy(response->body, body);
    } else {
        response->body_len = 0;
        response->body = NULL;
    }

    return response;
}

// Function to free an HttpResponse
void free_http_response(HttpResponse* response)
{
    if (!response) {
        return;
    }

    // Free the status string
    free(response->status);

    // Free the headers
    free_headers(response->headers, response->headers_len);

    // Free the body
    free(response->body);
}

int copy_http_response(HttpResponse* first, HttpResponse* second)
{
    if (!first || !second) {
        return -1; // Error: Null pointer passed
    }

    // Initialize second to avoid copying over existing data
    second->status = NULL;
    second->headers_len = 0;
    second->headers = NULL;
    second->body_len = 0;
    second->body = NULL;

    // Copy status
    if (first->status) {
        second->status = strdup(first->status);
        if (!second->status) {
            return -1; // Error: Memory allocation failed
        }
    }

    // Copy headers
    second->headers_len = first->headers_len;
    if (first->headers_len > 0 && first->headers) {
        second->headers = (char**)malloc(first->headers_len * sizeof(char*));
        if (!second->headers) {
            free(second->status);
            return -1; // Error: Memory allocation failed
        }

        for (size_t i = 0; i < first->headers_len; i++) {
            second->headers[i] = strdup(first->headers[i]);
            if (!second->headers[i]) {
                // Cleanup previously allocated memory
                for (size_t j = 0; j < i; j++) {
                    free(second->headers[j]);
                }
                free(second->headers);
                free(second->status);
                return -1; // Error: Memory allocation failed
            }
        }
    }

    // Copy body
    second->body_len = first->body_len;
    if (first->body_len > 0 && first->body) {
        second->body = (char*)malloc(first->body_len);
        if (!second->body) {
            // Cleanup allocated memory
            for (size_t i = 0; i < second->headers_len; i++) {
                free(second->headers[i]);
            }
            free(second->headers);
            free(second->status);
            return -1; // Error: Memory allocation failed
        }
        memcpy(second->body, first->body, first->body_len);
    }

    return 0; // Success
}
