#ifndef EVENTS_H
#define EVENTS_H

#include "uuid4.h"
#include <time.h>

enum {
    NewMessageEventType
};

extern const char* event_type_strs[];

typedef struct {
    int event_type;
} EventBase;

typedef struct {
    char uuid[UUID4_LEN];
    char* data;
    time_t creation_date;
} MsgWithMetaInfo;

int create_msg_with_meta_info(MsgWithMetaInfo* msg, char* uuid, char* data, time_t creation_date);
void free_message_with_metainfo(MsgWithMetaInfo* msg);

typedef struct {
    EventBase base;
    size_t msgs_len;
    MsgWithMetaInfo* msgs;
} EventNewMessage;

int create_event_new_message(EventNewMessage* ev, MsgWithMetaInfo* msg);

int copy_event_new_message(EventNewMessage* ev1, EventNewMessage** ev2);
int copy_event_base(EventBase* ev1, EventBase** ev2);

char* convert_event_new_message_to_json(EventNewMessage* ev);

char* convert_event_base_to_json(EventBase* ev);

void free_event_new_message(EventNewMessage* ev);
void free_event_base(EventBase* ev);

#endif
