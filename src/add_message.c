#include "add_message.h"
#include "db.h"
#include "event_bus.h"
#include "events.h"
#include "log.h"
#include "uuid4.h"
#include "yyjson.h"
#include <stdlib.h>

// Free function for AddMessageInput
void free_add_message_input(AddMessageInput* self)
{
    if (!self)
        return; // Guard against NULL pointer
    free(self->session_key);
    free(self->msg);
    free(self->receiver_uuid);
}

// JSON parsing function for AddMessageInput
int parse_json_to_add_message_input(size_t json_len, char json[json_len], AddMessageInput* model)
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

    // Extract "session_key" field
    yyjson_val* session_key_val = yyjson_obj_get(root, "session_key");
    if (!yyjson_is_str(session_key_val)) {
        yyjson_doc_free(doc);
        return -5; // Error: "session_key" is missing or not a string
    }

    // Extract "msg" field
    yyjson_val* msg_val = yyjson_obj_get(root, "msg");
    if (!yyjson_is_str(msg_val)) {
        yyjson_doc_free(doc);
        return -4; // Error: "msg" is missing or not a string
    }

    // Extract "reciever_uuid" field
    yyjson_val* receiver_uuid_val = yyjson_obj_get(root, "receiver_uuid");
    if (!yyjson_is_str(receiver_uuid_val)) {
        yyjson_doc_free(doc);
        return -5; // Error: "reciever_uuid" is missing or not a string
    }

    // Allocate memory for msg and reciever_uuid (caller must free later)
    model->session_key = strdup(yyjson_get_str(session_key_val));
    model->msg = strdup(yyjson_get_str(msg_val));
    model->receiver_uuid = strdup(yyjson_get_str(receiver_uuid_val));

    // Free the JSON document
    yyjson_doc_free(doc);

    // Check allocation success
    if (!model->session_key || !model->msg || !model->receiver_uuid) {
        return -6; // Error: Memory allocation failed
    }

    return 0; // Success
}

int add_message_route(HttpRequest* req, HttpResponse* res)
{
    LogInfo("add_message_route executed");

    // Parse input JSON
    AddMessageInput input;
    if (parse_json_to_add_message_input(req->body_len, req->body, &input)) {
        LogErr("Incorrect Json on Input: Json = '%.*s'", (int)req->body_len, req->body);
        create_http_response(res, "400", NULL, 0, NULL);
        return 0;
    }

    LogTrace("Input Body: %.*s", (int)req->body_len, req->body);
    LogTrace("Session Key = '%s'; Message = '%s'; Receiver UUID = '%s'", input.session_key, input.msg, input.receiver_uuid);

    int user_id;
    if (get_user_id_by_session_key(input.session_key, &user_id)) {
        LogErr("Cant find such session key in db: session_key = '%s'", input.session_key);
        create_http_response(res, "403", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    int receiver_id;
    if (get_user_id_by_uuid(input.receiver_uuid, &receiver_id)) {
        LogErr("Cant find such reciever uuid in db: receiver_uuid = '%s'", input.receiver_uuid);
        create_http_response(res, "404", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    char message_uuid[UUID4_LEN];
    uuid4_generate(message_uuid);

    time_t current_time = time(NULL);

    const Message message = {
        .created_at = current_time,
        .updated_at = current_time,
        .uuid = message_uuid,
        .sender_id = user_id,
        .receiver_id = receiver_id,
        .data = input.msg,
    };

    LogTrace("Message struct: uuid = '%s'; sender_id = '%d'; receiver_id = '%d'; data = '%s'\n",
        message.uuid, message.sender_id, message.receiver_id, message.data);

    if (add_message_to_db(&message)) {
        LogErr("Cant add message to db");
        create_http_response(res, "500", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    MsgWithMetaInfo* ev_msg = malloc(sizeof(MsgWithMetaInfo));
    if (!ev_msg) {
        LogErr("Cant alloc memory for message for event");
        create_http_response(res, "500", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    if (create_msg_with_meta_info(ev_msg, message_uuid, input.msg, current_time)) {
        LogErr("Cant create msg with meta info for event");
        create_http_response(res, "500", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    EventNewMessage* ev = malloc(sizeof(EventNewMessage));
    if (!ev) {
        LogErr("Cant alloc memory for new message event");
        create_http_response(res, "500", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    if (create_event_new_message(ev, ev_msg)) {
        LogErr("Cant create new message event");
        create_http_response(res, "500", NULL, 0, NULL);
        free_add_message_input(&input);
        return 0;
    }

    if (add_new_event_to_queue_by_user_id(global_event_bus, receiver_id, (EventBase*)ev)) {
        LogWarn("Cant send message to bus");
        free_event_base((EventBase*)ev);
    }

    // Successfully created the message
    create_http_response(res, "200", NULL, 0, "message added");
    free_add_message_input(&input);

    LogInfo("message added successfully");

    return 0;
}
