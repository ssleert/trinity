#ifndef AUTH_USER_H
#define AUTH_USER_H

#include "http.h"

typedef struct {
    char* nickname;
    char* password;
} AuthUserInput;

void free_auth_user_input(AuthUserInput* self);

int parse_json_to_auth_user_input(size_t json_len, char json[json_len], AuthUserInput* model);

int auth_user_route(HttpRequest* req, HttpResponse* res);

#endif
