#ifndef DB_H
#define DB_H

#include "time.h"

typedef struct {
  char* uuid;
  char* nickname;
  char* password_hash;
  char* password_hash_pow;
} User;

int init_db(const char* filename);

int add_user_to_db(const User* user);

#endif
