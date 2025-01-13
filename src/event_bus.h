#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "queue.h"
#include "events.h"
#include <pthread.h>

typedef struct {
  int connected; // if not connected can be reused for another user
  int user_id;
  Queue* event_queue;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} UserIdWithQueue;

typedef struct {
  size_t user_queues_len;
  UserIdWithQueue* user_queues;
  pthread_mutex_t mutex;
} EventBus;

int init_global_event_bus(void);

int create_event_bus(EventBus* eb);

// returns user_queue index
// and < 0 on error
int add_new_user_id_with_queue_to_event_bus(EventBus* eb, int user_id);

// first finds not connected UserIdWithQueue and reuse it
// if all connected add new to array
int add_new_event_to_queue_by_user_id(EventBus* eb, int user_id, EventBase* ev);

void disconnect_from_queue_by_index(EventBus* eb, size_t index);

int get_or_wait_for_new_event_in_queue_by_index(EventBus* eb, size_t index, EventBase** ev);

extern EventBus* global_event_bus;

#endif
