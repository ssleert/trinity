#ifndef UTILS_H
#define UTILS_H

#include "http.h"

int is_it_event_subscription(HttpRequest* req);

char* xsprintf(const char* fmt, ...);

#endif
