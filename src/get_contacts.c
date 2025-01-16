#include "get_contacts.h"
#include "db.h"
#include "log.h"
#include "yyjson.h"
#include <stdlib.h>

int parse_json_to_get_contacts_input(size_t json_len, char json[json_len], GetContactsInput* model) {
    if (!json || !model) {
        return -1; // Error: Invalid input
    }

    yyjson_doc* doc = yyjson_read(json, json_len, 0);
    if (!doc) {
        return -2; // Error: Failed to parse JSON
    }

    yyjson_val* root = yyjson_doc_get_root(doc);
    yyjson_val* session_key_val = yyjson_obj_get(root, "session_key");
    if (!yyjson_is_str(session_key_val)) {
        yyjson_doc_free(doc);
        return -3; // Error: Missing or invalid session_key
    }

    model->session_key = strdup(yyjson_get_str(session_key_val));
    yyjson_doc_free(doc);

    if (!model->session_key) {
        return -4; // Error: Memory allocation failed
    }

    return 0; // Success
}

int get_contacts_route(HttpRequest* req, HttpResponse* res) {
    LogInfo("get_contacts_route executed");

    // Parse the session_key from the request body
    GetContactsInput input;
    if (parse_json_to_get_contacts_input(req->body_len, req->body, &input) != 0) {
        create_http_response(res, "400", NULL, 0, "Invalid request: missing or invalid session_key");
        LogErr("Failed to parse JSON body");
        return 0;
    }

    // Retrieve senders' UUIDs and nicknames
    SenderUuidAndNickname* senders = NULL;
    size_t senders_len = 0;
    if (get_all_senders_uuid_and_nicknames_by_user_id_from_session_key(input.session_key, &senders, &senders_len) != 0) {
        create_http_response(res, "404", NULL, 0, "Session not found or database error");
        LogWarn("Failed to retrieve senders for session key: %s", input.session_key);
        free(input.session_key);
        return 0;
    }

    free(input.session_key);

    // Serialize the response JSON
    size_t buffer_size = 3; // Initial size for "[" and "]" and "\0"
    for (size_t i = 0; i < senders_len; ++i) {
        buffer_size += strlen(senders[i].uuid) + strlen(senders[i].nickname) + 40; // UUID, nickname, quotes, braces, and commas
    }

    char* json_response = malloc(buffer_size);
    if (!json_response) {
        create_http_response(res, "500", NULL, 0, "Internal server error");
        LogErr("Failed to allocate memory for JSON response");
        for (size_t i = 0; i < senders_len; ++i) {
            free(senders[i].uuid);
            free(senders[i].nickname);
        }
        free(senders);
        return 0;
    }

    char* cursor = json_response;
    cursor += sprintf(cursor, "[");
    for (size_t i = 0; i < senders_len; ++i) {
        cursor += sprintf(cursor, "{\"uuid\":\"%s\",\"nickname\":\"%s\"}%s",
                          senders[i].uuid, senders[i].nickname, i < senders_len - 1 ? "," : "");
        free(senders[i].uuid);
        free(senders[i].nickname);
    }
    cursor[0] = ']';
    cursor[1] = '\0';

    free(senders);

    // Create the HTTP response
    create_http_response(res, "200", NULL, 0, json_response);
    free(json_response);

    LogInfo("Successfully retrieved and serialized contacts");
    return 0;
}
