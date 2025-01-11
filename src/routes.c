#include "routes.h"
#include "create_user.h"
#include <string.h>

int exec_route_by_path(HttpRequest* req, HttpResponse* res) {
  return strcmp(req->path, "/register") == 0 ? create_user_route(req, res) :
         strcmp(req->path, "/swag") == 0     ? create_user_route(req, res) : -255;
}
