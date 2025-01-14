#ifndef ROUTES_H
#define ROUTES_H

#include "http.h"
#include <pthread.h>

typedef struct {
    pthread_t thrd;
    HttpRequest* req;
    HttpResponse* res;
} RequestAndResponse;

int exec_route_by_path(HttpRequest* req, HttpResponse* res);
void* exec_route_by_path_in_thread(void* data);

#endif
