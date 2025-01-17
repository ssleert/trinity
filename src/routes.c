#include "routes.h"
#include "add_message.h"
#include "auth_user.h"
#include "create_user.h"
#include "event_subcribe.h"
#include "get_contacts.h"
#include "get_messages.h"
#include "http.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for close

int exec_route_by_path(HttpRequest* req, HttpResponse* res)
{
    char* path = strdup(req->path);
    if (!path) {
        return -256;
    }
    char* q_mark = strchr(path, '?');
    if (q_mark) {
        *q_mark = '\0';
    }

    // cors support
    if (strcmp(req->method, "OPTIONS") == 0) {

        LogInfo("cors request");
        create_http_response(res, "200", NULL, 0, NULL);
        return 0;
    }

    int rc = strcmp(path, "/register") == 0 ? create_user_route(req, res) : strcmp(path, "/login") == 0 ? auth_user_route(req, res)
        : strcmp(path, "/send") == 0                                                                       ? add_message_route(req, res)
        : strcmp(path, "/events/subscribe") == 0                                                           ? event_subcribe_route(req, res)
        : strcmp(path, "/contacts") == 0                                                                   ? get_contacts_route(req, res)
        : strcmp(path, "/messages") == 0                                                                   ? get_messages_route(req, res) : -255;

    free(path);
    return rc;
}

void* exec_route_by_path_in_thread(void* data)
{
    RequestAndResponse* req_and_res = data;

    int rc = exec_route_by_path(req_and_res->req, req_and_res->res);
    LogTrace("Route in thread Executed: rc = %d", rc);

    LogTrace("im alive");
    close(req_and_res->req->socket);
    free(req_and_res->req);
    LogTrace("im alive");

    return 0;
}
