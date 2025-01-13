#include "routes.h"
#include "create_user.h"
#include "auth_user.h"
#include "add_message.h"
#include "event_subcribe.h"
#include "log.h"
#include <unistd.h>       // for close
#include <string.h>
#include <stdlib.h>

int exec_route_by_path(HttpRequest* req, HttpResponse* res) {
  return strcmp(req->path, "/register") == 0         ? create_user_route(req, res)    :
         strcmp(req->path, "/login") == 0            ? auth_user_route(req, res)      : 
         strcmp(req->path, "/send") == 0             ? add_message_route(req, res)    :
         strcmp(req->path, "/events/subscribe") == 0 ? event_subcribe_route(req, res) : -255;
}

void* exec_route_by_path_in_thread(void* data) {
  RequestAndResponse* req_and_res = data;

  int rc = exec_route_by_path(req_and_res->req, req_and_res->res);
  LogTrace("Route in thread Executed: rc = %d", rc);
  
  LogTrace("im alive");
  free(req_and_res->req);
  LogTrace("im alive");

  return 0;
}

