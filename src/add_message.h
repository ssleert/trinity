#ifndef ADD_MESSAGE_H
#define ADD_MESSAGE_H

#include "http.h"

typedef struct {
  char* session_key;
  char* msg;
  char* receiver_uuid;
} AddMessageInput;

void free_add_message_input(AddMessageInput* self);
int parse_json_to_add_message_input(size_t json_len, char json[json_len], AddMessageInput* model);

int add_message_route(HttpRequest* req, HttpResponse* _);

#endif
