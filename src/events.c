#include "events.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// Function to create MsgWithMetaInfo structure
int create_msg_with_meta_info(MsgWithMetaInfo* msg, char* uuid, char* data, time_t creation_date) {
    if (!msg || !uuid || !data) {
        return -1; // Invalid arguments
    }

    // Copy the UUID
    strncpy(msg->uuid, uuid, UUID4_LEN);
    msg->uuid[UUID4_LEN - 1] = '\0'; // Ensure null-termination

    // Allocate memory for the message data
    msg->data = strdup(data);
    if (!msg->data) {
        return -1; // Memory allocation failure
    }

    // Set creation date
    msg->creation_date = creation_date;

    return 0; // Success
}

// Function to free MsgWithMetaInfo structure
void free_message_with_metainfo(MsgWithMetaInfo* msg) {
    if (msg) {
        free(msg->data); // Free the dynamically allocated data
    }
}

// Function to create EventNewMessage structure
int create_event_new_message(EventNewMessage* ev, MsgWithMetaInfo* msg) {
    if (!ev || !msg) {
        return -1; // Invalid arguments
    }

    // Initialize the base event type (NewMessageEventType)
    ev->base.event_type = NewMessageEventType;

    // Allocate memory for the messages array and copy the provided message
    ev->msgs_len = 1; // Only one message in the event for now
    ev->msgs = (MsgWithMetaInfo*)malloc(sizeof(MsgWithMetaInfo) * ev->msgs_len);
    if (!ev->msgs) {
        return -1; // Memory allocation failure
    }

    // Copy the message into the event
    ev->msgs[0] = *msg;

    return 0; // Success
}

int copy_event_new_message(EventNewMessage* ev1, EventNewMessage** ev2) {
    if (ev1 == NULL || ev2 == NULL) {
        return -1; // Error: Invalid input
    }

    // Allocate memory for the new event
    *ev2 = (EventNewMessage*)malloc(sizeof(EventNewMessage));
    if (*ev2 == NULL) {
        return -1; // Error: Memory allocation failed
    }

    // Copy the base event type
    (*ev2)->base.event_type = ev1->base.event_type;

    // Copy the number of messages
    (*ev2)->msgs_len = ev1->msgs_len;

    // Allocate memory for the messages array
    if (ev1->msgs_len > 0) {
        (*ev2)->msgs = (MsgWithMetaInfo*)malloc(ev1->msgs_len * sizeof(MsgWithMetaInfo));
        if ((*ev2)->msgs == NULL) {
            free(*ev2);
            *ev2 = NULL;
            return -1; // Error: Memory allocation failed
        }

        // Copy each message
        for (size_t i = 0; i < ev1->msgs_len; i++) {
            // Copy the UUID
            strncpy((*ev2)->msgs[i].uuid, ev1->msgs[i].uuid, UUID4_LEN);

            // Copy the data string
            if (ev1->msgs[i].data != NULL) {
                (*ev2)->msgs[i].data = strdup(ev1->msgs[i].data);
                if ((*ev2)->msgs[i].data == NULL) {
                    // Cleanup and return on error
                    for (size_t j = 0; j < i; j++) {
                        free((*ev2)->msgs[j].data);
                    }
                    free((*ev2)->msgs);
                    free(*ev2);
                    *ev2 = NULL;
                    return -1; // Error: Memory allocation failed
                }
            } else {
                (*ev2)->msgs[i].data = NULL;
            }

            // Copy the creation date
            (*ev2)->msgs[i].creation_date = ev1->msgs[i].creation_date;
        }
    } else {
        (*ev2)->msgs = NULL;
    }

    return 0; // Success
}

int copy_event_base(EventBase* ev1, EventBase** ev2) {
    if (ev1 == NULL) return -1;

    switch (ev1->event_type) {
        case NewMessageEventType: return copy_event_new_message((EventNewMessage*)ev1, (EventNewMessage**)ev2); 
        default: return -1;
    }
}

char* convert_event_new_message_to_json(EventNewMessage* ev) {
    if (!ev) {
        return NULL; // Return error if input is invalid
    }

    char* json = NULL;
    char* temp = NULL;

    // Start the JSON string
    json = xsprintf("{\"event_type\":%d,\"msgs\":[",
                    ev->base.event_type);

    if (!json) {
        return NULL; // Memory allocation or formatting failed
    }

    // Loop through the messages and append their JSON representation
    for (size_t i = 0; i < ev->msgs_len; ++i) {
        MsgWithMetaInfo* msg = &ev->msgs[i];

        // Format the message as JSON
        temp = xsprintf("{\"uuid\":\"%s\",\"data\":\"%s\",\"creation_date\":%ld}",
                        msg->uuid,
                        msg->data ? msg->data : "",
                        msg->creation_date);
        if (!temp) {
            free(json); // Cleanup on failure
            return NULL;
        }

        // Append the formatted message to the JSON string
        char* new_json = xsprintf("%s%s%s", json, (i > 0 ? "," : ""), temp);
        free(temp); // Free temp buffer after appending
        free(json); // Free old json before assigning the new one

        if (!new_json) {
            return NULL; // Memory allocation failed
        }

        json = new_json;
    }

    // Close the JSON array and object
    temp = xsprintf("%s]}", json);
    free(json); // Free the old json buffer

    if (!temp) {
        return NULL; // Memory allocation failed
    }

    return temp; // Return the complete JSON string
}

char* convert_event_base_to_json(EventBase* ev) {
    if (ev == NULL) return NULL;

    switch (ev->event_type) {
        case NewMessageEventType: return convert_event_new_message_to_json((EventNewMessage*)ev); 
        default: return NULL;
    }
}


// Function to free EventNewMessage structure
void free_event_new_message(EventNewMessage* ev) {
    if (ev) {
        // Free the messages array
        for (size_t i = 0; i < ev->msgs_len; ++i) {
            free_message_with_metainfo(&ev->msgs[i]);
        }
        free(ev->msgs); // Free the message array itself
    }
}

void free_event_base(EventBase* ev) {
    if (ev == NULL) return;  // Check for null pointer

    switch (ev->event_type) {
        case NewMessageEventType:
            // Cast to the specific event type (EventNewMessage) and call the appropriate free function
            free_event_new_message((EventNewMessage*)ev);
            break;
        
        // Add cases for other event types as needed in the future
        default:
            // Handle unknown event types, if necessary
            free(ev);
            break;
    }
}

char* convert_event_new_message_to_json(EventNewMessage* ev);
