#include "get_messages.h"
#include "log.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_url_params_to_get_messages_input(const char* path, GetMessagesInput* model) {
    if (!path || !model) {
        return -1; // Invalid input
    }

    // Initialize the model with defaults
    model->session_key = NULL;
    model->limit = 0;
    model->offset = 0;

    // Find the start of the query parameters
    const char* query_start = strchr(path, '?');
    if (!query_start) {
        return -1; // No query parameters found
    }
    query_start++; // Move past the '?' character

    char param[MAX_PARAM_LENGTH];
    const char* current = query_start;

    while (*current) {
        // Extract the key
        const char* key_start = current;
        const char* key_end = strchr(key_start, '=');
        if (!key_end) {
            break; // Malformed query string
        }

        size_t key_len = key_end - key_start;
        if (key_len >= MAX_PARAM_LENGTH) {
            return -1; // Key too long
        }

        strncpy(param, key_start, key_len);
        param[key_len] = '\0';

        const char* value_start = key_end + 1;
        const char* value_end = strchr(value_start, '&');
        if (!value_end) {
            value_end = value_start + strlen(value_start);
        }

        size_t value_len = value_end - value_start;
        if (value_len >= MAX_PARAM_LENGTH) {
            return -1; // Value too long
        }

        char value[MAX_PARAM_LENGTH];
        strncpy(value, value_start, value_len);
        value[value_len] = '\0';

        // Assign the value to the appropriate field in the model
        if (strcmp(param, "session_key") == 0) {
            model->session_key = strdup(value);
        } else if (strcmp(param, "limit") == 0) {
            model->limit = atoi(value);
        } else if (strcmp(param, "offset") == 0) {
            model->offset = atoi(value);
        } else if (strcmp(param, "user_uuid") == 0) {
            model->user_uuid = strdup(value);
        }

        current = value_end;
        if (*current == '&') {
            current++; // Move past the '&' character
        }
    }

    // Validate that required fields are set
    if (!model->session_key) {
        return -1; // session_key is mandatory
    }

    return 0; // Success
}

int get_messages_route(HttpRequest* req, HttpResponse* res) {
    GetMessagesInput input = {0};
    if (parse_url_params_to_get_messages_input(req->path, &input)) { 
        LogErr("Incorrect Url params on Input: URL = '%s'", req->path);
        create_http_response(res, "400", NULL, 0, NULL);
        return 0;
    }

    // Parse the URL parameters
    if (parse_url_params_to_get_messages_input(req->path, &input)) {
        LogErr("Incorrect URL params on Input: URL = '%s'", req->path);
        create_http_response(res, "400", NULL, 0, NULL);
        return 0;
    }

    // Validate essential parameters
    if (!input.session_key || !input.user_uuid || input.limit <= 0 || input.offset < 0) {
        LogErr("Invalid input parameters: session_key = '%s', user_uuid = '%s', limit = %d, offset = %d",
               input.session_key, input.user_uuid, input.limit, input.offset);
        create_http_response(res, "400", NULL, 0, NULL);
        goto cleanup;
    }

    // Retrieve messages from the database
    MessageWithTimeAndData* msgs = NULL;
    size_t msgs_len = 0;
    if (get_messages_by_reciever_user_id_from_session_key_and_sender_user_uuid(
            input.session_key, input.user_uuid, input.offset, input.limit, &msgs, &msgs_len) != EXIT_SUCCESS) {
        LogErr("Failed to retrieve messages from database.");
        create_http_response(res, "500", NULL, 0, NULL);
        goto cleanup;
    }

    // Calculate the total size for the JSON response
    size_t json_size = 2; // For the opening and closing brackets of the JSON array
    for (size_t i = 0; i < msgs_len; i++) {
        json_size += snprintf(NULL, 0, 
            "{\"data\":\"%s\",\"created_at\":%ld,\"updated_at\":%ld},",
            msgs[i].data, (long)msgs[i].created_at, (long)msgs[i].updated_at);
    }

    if (msgs_len > 0) {
        json_size -= 1; // Remove the trailing comma
    }

    char* json_response = malloc(json_size + 1);
    if (!json_response) {
        LogErr("Memory allocation for JSON response failed.");
        create_http_response(res, "500", NULL, 0, NULL);
        goto cleanup_msgs;
    }

    // Build the JSON response
    char* ptr = json_response;
    *ptr++ = '['; // Opening bracket
    for (size_t i = 0; i < msgs_len; i++) {
        int written = snprintf(ptr, json_size - (ptr - json_response),
            "{\"data\":\"%s\",\"created_at\":%ld,\"updated_at\":%ld},",
            msgs[i].data, (long)msgs[i].created_at, (long)msgs[i].updated_at);
        ptr += written;
    }

    if (msgs_len > 0) {
        ptr--; // Remove the trailing comma
    }
    *ptr++ = ']'; // Closing bracket
    *ptr = '\0'; // Null-terminate the string

    // Send JSON response
    create_http_response(res, "200", NULL, 0, json_response);

    free(json_response);

cleanup_msgs:
    // Free messages array and its contents
    for (size_t i = 0; i < msgs_len; i++) {
        free(msgs[i].data);
    }
    free(msgs);

cleanup:
    // Free input fields
    free(input.session_key);
    free(input.user_uuid);

    return 0;
} 
