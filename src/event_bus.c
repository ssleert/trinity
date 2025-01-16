#include "event_bus.h"
#include "events.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EventBus* global_event_bus = NULL;

int init_global_event_bus(void)
{
    LogTrace("Initializing global event bus.");
    global_event_bus = malloc(sizeof(*global_event_bus));
    if (global_event_bus == NULL) {
        LogErr("Failed to allocate memory for global event bus.");
        return -1;
    }

    return create_event_bus(global_event_bus);
}

// Create the event bus and initialize it
int create_event_bus(EventBus* eb)
{
    if (!eb) {
        LogErr("Null pointer provided for event bus.");
        return -1;
    }

    LogTrace("Creating event bus.");
    eb->user_queues_len = 0;
    eb->user_queues = malloc(1);
    if (!eb->user_queues) {
        LogErr("Failed to allocate memory for user queues.");
        return -1;
    }
    pthread_mutex_init(&eb->mutex, NULL);

    LogTrace("Event bus created successfully.");
    return 0;
}

// Add a new user id with its queue to the event bus
// returns user_queue index or < 0 on error
int add_new_user_id_with_queue_to_event_bus(EventBus* eb, int user_id)
{
    LogTrace("Adding new user with ID: %d to event bus.", user_id);
    pthread_mutex_lock(&eb->mutex);

    // Check if we need to add a new user or reuse an existing one
    for (size_t i = 0; i < eb->user_queues_len && eb->user_queues_len > 0; ++i) {
        if (eb->user_queues[i].connected == 0) {
            LogTrace("Reusing existing slot for user ID: %d at index: %zu.", user_id, i);
            eb->user_queues[i].user_id = user_id;
            eb->user_queues[i].event_queue = createQueue();
            eb->user_queues[i].connected = 1;
            pthread_mutex_init(&eb->user_queues[i].mutex, NULL);
            pthread_cond_init(&eb->user_queues[i].cond, NULL);

            pthread_mutex_unlock(&eb->mutex);
            return i;
        }
    }

    // Add a new user id with a new queue
    size_t new_len = eb->user_queues_len + 1;
    LogTrace("Adding new slot for user ID: %d. New length: %zu.", user_id, new_len);
    eb->user_queues = realloc(eb->user_queues, new_len * sizeof(UserIdWithQueue));
    if (!eb->user_queues) {
        LogErr("Failed to allocate memory for new user queue.");
        pthread_mutex_unlock(&eb->mutex);
        return -1;
    }

    eb->user_queues[new_len - 1].user_id = user_id;
    eb->user_queues[new_len - 1].event_queue = createQueue();
    eb->user_queues[new_len - 1].connected = 1;
    pthread_mutex_init(&eb->user_queues[new_len - 1].mutex, NULL);
    pthread_cond_init(&eb->user_queues[new_len - 1].cond, NULL);

    eb->user_queues_len = new_len;

    LogTrace("User ID: %d added successfully at index: %zu.", user_id, new_len - 1);
    pthread_mutex_unlock(&eb->mutex);
    return new_len - 1;
}

// Add an event to the queue of the user identified by user_id
// Returns 0 if successful, or < 0 on error
int add_new_event_to_queue_by_user_id(EventBus* eb, int user_id, EventBase* ev)
{
    LogTrace("Adding new event for user ID: %d.", user_id);

    // Find the corresponding user queue
    int found = 0;
    for (size_t i = 0; i < eb->user_queues_len; ++i) {
        if (eb->user_queues[i].connected && eb->user_queues[i].user_id == user_id) {
            LogTrace("Found user ID: %d at index: %zu. Adding event to queue.", user_id, i);
            pthread_mutex_lock(&eb->user_queues[i].mutex);
            EventBase* ev_copy;
            copy_event_base(ev, &ev_copy);
            enqueue(eb->user_queues[i].event_queue, ev_copy);
            pthread_cond_signal(&eb->user_queues[i].cond); // Notify the user of the new event
            pthread_mutex_unlock(&eb->user_queues[i].mutex);
            found = 1;
        }
    }

    if (!found) {
        LogWarn("User ID: %d not found or not connected.", user_id);
        return -1; // User ID not found or not connected
    }

    LogTrace("Event added successfully for user ID: %d.", user_id);
    return 0;
}

// Disconnect a user by its index in the event bus
void disconnect_from_queue_by_index(EventBus* eb, size_t index)
{
    LogTrace("Disconnecting user at index: %zu.", index);
    pthread_mutex_lock(&eb->mutex);

    if (index < eb->user_queues_len && eb->user_queues[index].connected) {
        LogTrace("User at index: %zu is connected. Proceeding with disconnection.", index);
        // Disconnect the user
        eb->user_queues[index].connected = 0;

        // Free the queue resources
        Queue* event_queue = eb->user_queues[index].event_queue;
        free(event_queue);
        eb->user_queues[index].event_queue = NULL;

        // Optionally, free mutex and condition variable resources
        pthread_mutex_destroy(&eb->user_queues[index].mutex);
        pthread_cond_destroy(&eb->user_queues[index].cond);
    } else {
        LogWarn("Invalid index or user already disconnected: %zu.", index);
    }

    pthread_mutex_unlock(&eb->mutex);
    LogTrace("User at index: %zu disconnected successfully.", index);
}

// Wait for a new event in a user's queue and retrieve it by its index
// Returns 0 if an event is successfully retrieved, < 0 on error
int get_or_wait_for_new_event_in_queue_by_index(EventBus* eb, size_t index, EventBase** ev)
{
    LogTrace("Waiting for new event in queue at index: %zu.", index);

    if (index >= eb->user_queues_len || !eb->user_queues[index].connected) {
        LogWarn("Invalid index or disconnected user at index: %zu.", index);
        return -1; // Invalid index or disconnected user
    }

    pthread_mutex_lock(&eb->user_queues[index].mutex);

    while (isEmpty(eb->user_queues[index].event_queue)) {
        LogTrace("Queue at index: %zu is empty. Waiting for event.", index);
        pthread_cond_wait(&eb->user_queues[index].cond, &eb->user_queues[index].mutex);
    }

    *ev = dequeue(eb->user_queues[index].event_queue); // Retrieve the event
    LogTrace("Event retrieved successfully from queue at index: %zu.", index);

    pthread_mutex_unlock(&eb->user_queues[index].mutex);
    return 0;
}
