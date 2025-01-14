#include "auth_user.h"
#include "crypto.h"
#include "db.h"
#include "log.h"
#include "sha256.h"
#include "uuid4.h"
#include "yyjson.h"
#include <stdlib.h>

void free_auth_user_input(AuthUserInput* self)
{
    free(self->nickname);
    free(self->password);
}

int parse_json_to_auth_user_input(size_t json_len, char json[json_len], AuthUserInput* model)
{
    if (!json || !model) {
        return -1; // Error: Invalid input
    }

    // Parse the JSON string into a yyjson document
    yyjson_doc* doc = yyjson_read(json, json_len, 0);
    if (!doc) {
        return -2; // Error: Failed to parse JSON
    }

    // Get the root JSON object
    yyjson_val* root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return -3; // Error: Root is not a JSON object
    }

    // Extract "nickname" field
    yyjson_val* nickname_val = yyjson_obj_get(root, "nickname");
    if (!yyjson_is_str(nickname_val)) {
        yyjson_doc_free(doc);
        return -4; // Error: "nickname" is missing or not a string
    }

    // Extract "password" field
    yyjson_val* password_val = yyjson_obj_get(root, "password");
    if (!yyjson_is_str(password_val)) {
        yyjson_doc_free(doc);
        return -5; // Error: "password" is missing or not a string
    }

    // Allocate memory for nickname and password (caller must free later)
    model->nickname = strdup(yyjson_get_str(nickname_val));
    model->password = strdup(yyjson_get_str(password_val));

    // Free the JSON document
    yyjson_doc_free(doc);

    // Check allocation success
    if (!model->nickname || !model->password) {
        return -6; // Error: Memory allocation failed
    }

    return 0; // Success
}

int auth_user_route(HttpRequest* req, HttpResponse* res)
{
    LogInfo("auth_user_route executed");

    AuthUserInput input;
    if (parse_json_to_auth_user_input(req->body_len, req->body, &input)) {
        LogErr("Incorrect Json on Input: Json = '%.*s'", (int)req->body_len, req->body);
        create_http_response(res, "400", NULL, 0, NULL);
        return 0;
    };

    LogTrace("Input Body: %.*s", (int)req->body_len, req->body);
    LogTrace("Nickname = '%s'; Password = '%s'", input.nickname, input.password);

    char stored_password_hash[SHA256_HEX_SIZE];
    char stored_password_hash_pow[UUID4_LEN];
    int user_id;

    // Retrieve the stored password hash and proof-of-work (POW) for the user
    if (get_user_password_hash_and_pow_and_id_by_nickname_from_db(
            input.nickname, stored_password_hash, stored_password_hash_pow, &user_id)
        != EXIT_SUCCESS) {
        create_http_response(res, "404", NULL, 0, "User not found or database error");
        free_auth_user_input(&input);
        LogWarn("Authentication failed: User not found or database error");
        return 0;
    }

    LogTrace(
        "Retrieved stored_password_hash = '%s', stored_password_hash_pow = '%s' for nickname '%s'",
        stored_password_hash, stored_password_hash_pow, input.nickname);

    char computed_password_hash[SHA256_HEX_SIZE];

    // Compute the hash of the user's provided password with the stored proof-of-work
    if (hash_user_password_with_pow(computed_password_hash, input.password, stored_password_hash_pow)) {
        create_http_response(res, "500", NULL, 0, "Internal Server Error");
        free_auth_user_input(&input);
        LogErr("Failed to compute password hash during authentication");
        return 0;
    }

    LogTrace("Computed password hash = '%s'", computed_password_hash);

    // Compare the computed hash with the stored hash
    if (strcmp(computed_password_hash, stored_password_hash) != 0) {
        create_http_response(res, "401", NULL, 0, "Invalid credentials");
        free_auth_user_input(&input);
        LogWarn("Authentication failed: Invalid credentials for user: %s", input.nickname);
        return 0;
    }

    char session_key[UUID4_LEN];
    uuid4_generate(session_key);

    const Session session = {
        .session_key = session_key,
        .user_id = user_id,
    };

    int rc = add_session_to_db(&session);
    if (rc) {
        create_http_response(res, "500", NULL, 0, "Internal Server Error");
        free_auth_user_input(&input);
        LogErr("Failed to insert into db");
        return 0;
    }

    create_http_response(res, "200", NULL, 0, session_key);
    free_auth_user_input(&input);

    return 0;
}
