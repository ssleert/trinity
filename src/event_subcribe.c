#include "event_subcribe.h"
#include "db.h"
#include "event_bus.h"
#include "events.h"
#include "http.h"
#include "log.h"
#include "utils.h"
#include <string.h>
#include <sys/socket.h> // for socket
#include <unistd.h>

int event_subcribe_route(HttpRequest* req, HttpResponse* _)
{
    LogTrace("Starting event_subscribe_route");

    HttpResponse* res = malloc(sizeof(HttpResponse));
    if (res == NULL) {
        LogErr("Failed to allocate memory for HttpResponse");
        return -1;
    }

    LogTrace("HttpResponse allocated successfully");

    char* session_key = xsprintf("%.*s", (int)req->body_len, req->body);
    if (!session_key) {
        LogErr("Failed to parse session key from request body");
        create_http_response(res, "400", NULL, 0, NULL);
        http_response_write_to_socket(req->socket, res);
        free_http_response(res);
        return 0;
    }

    LogTrace("Session key parsed: %s", session_key);

    int user_id;
    if (get_user_id_by_session_key(session_key, &user_id)) {
        LogErr("Failed to get user ID for session key: %s", session_key);
        free(session_key);
        create_http_response(res, "403", NULL, 0, NULL);
        http_response_write_to_socket(req->socket, res);
        free_http_response(res);
        return 0;
    }

    LogTrace("User ID retrieved: %d", user_id);
    free(session_key);

    int queue_index = add_new_user_id_with_queue_to_event_bus(global_event_bus, user_id);
    if (queue_index < 0) {
        LogErr("Failed to add user ID %d to event bus", user_id);
        create_http_response(res, "500", NULL, 0, NULL);
        http_response_write_to_socket(req->socket, res);
        free_http_response(res);
        return 0;
    }

    LogTrace("User ID %d added to event bus with queue index %d", user_id, queue_index);

    create_http_response(
        res, "200",
        (const char*[]) {
            "X-Accel-Buffering: no",
            "Content-Type: text/event-stream",
            "Cache-Control: no-cache",
        },
        3,
        NULL);

    LogTrace("HTTP response for event stream created");

    if (http_response_write_to_socket(req->socket, res) < 0) {
        LogErr("Failed to write HTTP response to socket");
        free_http_response(res);
        return 0;
    }

    LogTrace("HTTP response sent to client");
    free_http_response(res);

    while (1) {
        LogTrace("Waiting for new event in queue index %d", queue_index);

        EventBase* ev;
        if (get_or_wait_for_new_event_in_queue_by_index(global_event_bus, queue_index, &ev)) {
            LogErr("Failed to retrieve event from queue index %d", queue_index);
            continue;
        }

        LogTrace("Event retrieved from queue index %d", queue_index);

        char* ev_json = convert_event_base_to_json(ev);
        if (!ev_json) {
            LogErr("Failed to convert event to JSON");
            free_event_base(ev);
            continue;
        }

        LogTrace("Event converted to JSON: %s", ev_json);

        char* sse_data = xsprintf("event: %s\ndata: %s\n\n", event_type_strs[ev->event_type], ev_json);

        if (send(req->socket, sse_data, strlen(sse_data), 0) <= 0) {
            perror("Cant write to event stream socket");
            LogErr("Failed to write event JSON to socket");
            free(ev_json);
            free(sse_data);
            free_event_base(ev);

            LogTrace("trying to disconnect from message queue");
            disconnect_from_queue_by_index(global_event_bus, queue_index);

            LogTrace("disconnected from message queue");
            return 0;
        }

        LogTrace("Event JSON sent to client: %s", ev_json);

        free(ev_json);
        free(sse_data);
        free_event_base(ev);
    }

    return 0;
}
