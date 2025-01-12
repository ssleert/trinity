#ifndef DB_H
#define DB_H

#include "time.h"

typedef struct {
  char* uuid;
  char* nickname;
  char* password_hash;
  char* password_hash_pow;
} User;

typedef struct {
  char* session_key;
  int   user_id;
} Session;

typedef struct {
  char* uuid;
  int   sender_id;
  int   receiver_id;
  char* data;
} Message;

int init_db(const char* filename);

int add_user_to_db(const User* user);

int get_user_password_hash_and_pow_and_id_by_nickname_from_db(const char* nickname, char* password_hash, char* password_hash_pow, int* id);

int get_user_id_by_uuid(const char* uuid, int* user_id);
int get_user_uuid_by_id(int user_id, char* uuid);

int add_session_to_db(const Session* session);
int get_sessions_id_by_user_id(int user_id, Session** sessions, size_t* sessions_len);
int get_user_id_by_session_key(const char* session_key, int* user_id);

int add_message_to_db(const Message* message);

#endif
