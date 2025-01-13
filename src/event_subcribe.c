#include "event_subcribe.h"
#include "http.h"
#include "log.h"
#include <unistd.h>
#include <sys/socket.h>  // for socket

int event_subcribe_route(HttpRequest* req, HttpResponse* _) {
  HttpResponse* res = malloc(sizeof(HttpResponse));
  if (res == NULL) {
    return -1;
  }

  create_http_response(
    res, "200", 
    (const char*[]){
      "X-Accel-Buffering: no",
      "Content-Type: text/event-stream",
      "Cache-Control: no-cache",
    }, 3,
    NULL
  );

  http_response_write_to_socket(req->socket, res);
  free_http_response(res);

  const char event_ping[] = "event: ping\n"
                            "data: {\"ping\": \"yes\"}"
                            "\n\n";

  while (1) {
    LogTrace("writing message to user");
    if (send(req->socket, event_ping, sizeof(event_ping)-1, 0) <= 0) {
      perror("Cant write to event stream socket");
      LogErr("failed to write to socket");
      return -1;
    };
    LogTrace("message to client written");
    sleep(1);
  }

  return 0;
}
