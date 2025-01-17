#ifndef GET_MESSAGES_H
#define GET_MESSAGES_H

#include "http.h"

#define MAX_PARAM_LENGTH 256

typedef struct{
  char* user_uuid;
  char* session_key;
  int limit;
  int offset;
} GetMessagesInput;

int parse_url_params_to_get_messages_input(const char* path, GetMessagesInput* model);

int get_messages_route(HttpRequest* req, HttpResponse* res);

#endif
