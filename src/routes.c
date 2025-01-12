#include "routes.h"
#include "create_user.h"
#include "auth_user.h"
#include "add_message.h"
#include <string.h>

int exec_route_by_path(HttpRequest* req, HttpResponse* res) {
  return strcmp(req->path, "/register") == 0 ? create_user_route(req, res) :
         strcmp(req->path, "/login") == 0    ? auth_user_route(req, res) : 
         strcmp(req->path, "/send") == 0     ? add_message_route(req, res) : -255;
}
