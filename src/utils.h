#ifndef UTILS_H
#define UTILS_H

#include "http.h"

int is_requested_http_event_stream(HttpRequest* req);
int is_it_event_subscription(HttpRequest* req);

#endif
