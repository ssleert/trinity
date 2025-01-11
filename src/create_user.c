#include "create_user.h"
#include "db.h"
#include "http.h"
#include "log.h"
#include "uuid4.h"
#include "yyjson.h"
#include "sha256.h"
#include "crypto.h"
#include <stdio.h>
#include <string.h>

void free_create_user_input(CreateUserInput* self) {
  free(self->nickname);
  free(self->password);
}

int parse_json_to_create_user_input(size_t json_len, char json[json_len], CreateUserInput* model) {
    if (!json || !model) {
        return -1; // Error: Invalid input
    }

    // Parse the JSON string into a yyjson document
    yyjson_doc *doc = yyjson_read(json, json_len, 0);
    if (!doc) {
        return -2; // Error: Failed to parse JSON
    }

    // Get the root JSON object
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!yyjson_is_obj(root)) {
        yyjson_doc_free(doc);
        return -3; // Error: Root is not a JSON object
    }

    // Extract "nickname" field
    yyjson_val *nickname_val = yyjson_obj_get(root, "nickname");
    if (!yyjson_is_str(nickname_val)) {
        yyjson_doc_free(doc);
        return -4; // Error: "nickname" is missing or not a string
    }

    // Extract "password" field
    yyjson_val *password_val = yyjson_obj_get(root, "password");
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

int create_user_route(HttpRequest* req, HttpResponse* res) {
    LogInfo("create_user_route executed");

    CreateUserInput input;
    if (parse_json_to_create_user_input(req->body_len, req->body, &input)) {
        LogErr("Incorrect Json on Input: Json = '%.*s'", (int)req->body_len, req->body);
        create_http_response(res, "400", NULL, 0, NULL);
        return 0;
    };

    LogTrace("Input Body: %.*s", (int)req->body_len, req->body);
    LogTrace("Nickname = '%s'; Password = '%s'", input.nickname, input.password);

    char user_uuid[UUID4_LEN];
    uuid4_generate(user_uuid);


    char password_hash_pow[UUID4_LEN];
    uuid4_generate(password_hash_pow);


    char user_password_hash[SHA256_HEX_SIZE];
    if (hash_user_password_with_pow(user_password_hash, input.password, password_hash_pow)) {
        LogErr("Cant hash password");
        create_http_response(res, "500", NULL, 0, NULL);
        return 0;
    }

    const User user = {
        .uuid = user_uuid,
        .nickname = input.nickname,
        .password_hash = user_password_hash,
        .password_hash_pow = password_hash_pow,
    };

    LogInfo(
        "user.uuid = %s; user.nickname = %s; user.password_hash = %s; user.password_hash_pow = %s",
        user.uuid, user.nickname, user.password_hash, user.password_hash_pow
    );

    int rc = add_user_to_db(&user);
    if (rc) {
        LogErr("Db error: rc = %d", rc);
        free_create_user_input(&input);
        create_http_response(res, "500", NULL, 0, NULL);
        return 0;
    }

    create_http_response(res, "200", NULL, 0, "user created");

    LogInfo("user created");

    return 0;
}
