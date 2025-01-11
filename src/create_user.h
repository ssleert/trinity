#ifndef CREATE_USER_H
#define CREATE_USER_H

#include "http.h"

typedef struct { 
  char* nickname;
  char* password;
} CreateUserInput;

void free_create_user_input(CreateUserInput* self);

int parse_json_to_create_user_input(size_t json_len, char json[json_len], CreateUserInput* model);

int create_user_route(HttpRequest* req, HttpResponse* res);

#endif
