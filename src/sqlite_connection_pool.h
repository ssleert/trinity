#ifndef SQLITE_CONNECTION_POOL_H
#define SQLITE_CONNECTION_POOL_H

#include "sqlite3.h"
#include "trinity.h"
#include <pthread.h>

#define SQLITE_CONN_POOL_SIZE NUM_THREADS

typedef struct {
    sqlite3* connections[SQLITE_CONN_POOL_SIZE];
    int in_use[SQLITE_CONN_POOL_SIZE];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} SQLiteConnectionPool;

int init_sqlite_connection_pool(SQLiteConnectionPool* pool, const char* db_path);
sqlite3* sqlite_get_connection(SQLiteConnectionPool* pool);
void sqlite_release_connection(SQLiteConnectionPool* pool, sqlite3* connection);
void sqlite_destroy_connection_pool(SQLiteConnectionPool* pool);

#endif
