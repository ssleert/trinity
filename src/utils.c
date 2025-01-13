#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>


int is_it_event_subscription(HttpRequest* req) { 
  return strcmp(req->path, "/events/subscribe");
}

char* xsprintf(const char *fmt, ...) {
    if (!fmt) {
        return NULL; // Return error if input is invalid
    }

    va_list args;
    va_start(args, fmt);

    // Determine the size of the formatted string
    int size = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (size < 0) {
        return NULL; // vsnprintf failed
    }

    // Allocate memory for the formatted string
    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        return NULL; // Memory allocation failed
    }

    // Write the formatted string into the allocated buffer
    va_start(args, fmt);
    int written = vsnprintf(buffer, size + 1, fmt, args);
    va_end(args);

    if (written < 0) {
        free(buffer); // Cleanup on failure
        return NULL;
    }

    return buffer; // Return the number of characters written
}
