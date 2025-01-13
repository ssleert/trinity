#include "utils.h"
#include <string.h>


int is_it_event_subscription(HttpRequest* req) { 
  return strcmp(req->path, "/events/subscribe");
}
