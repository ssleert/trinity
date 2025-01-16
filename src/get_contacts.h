#ifndef GET_CONTACTS_H
#define GET_CONTACTS_H

#include "http.h"

typedef struct {
    char* session_key;
} GetContactsInput;

int parse_json_to_get_contacts_input(size_t json_len, char json[json_len], GetContactsInput* model);
int get_contacts_route(HttpRequest* req, HttpResponse* res);

#endif
