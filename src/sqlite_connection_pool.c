#include "sqlite_connection_pool.h"
#include "log.h"

// Function to initialize the connection pool
int init_sqlite_connection_pool(SQLiteConnectionPool* pool, const char* db_path)
{
    if (!pool || !db_path) {
        return -1;
    }

    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);

    for (int i = 0; i < SQLITE_CONN_POOL_SIZE; i++) {
        if (sqlite3_open(db_path, &pool->connections[i]) != SQLITE_OK) {
            LogErr("Error opening SQLite database: %s\n", sqlite3_errmsg(pool->connections[i]));
            return -1;
        }
        pool->in_use[i] = 0;
    }

    return 0;
}

// Function to get a connection from the pool
sqlite3* sqlite_get_connection(SQLiteConnectionPool* pool)
{
    pthread_mutex_lock(&pool->mutex);

    LogTrace("getting connection from pool");
    while (1) {
        for (int i = 0; i < SQLITE_CONN_POOL_SIZE; i++) {
            LogTrace("check for connection %d access: in_use = %d", i, pool->in_use[i]);
            if (!pool->in_use[i]) {
                LogTrace("connection %d free", i);
                pool->in_use[i] = 1;
                pthread_mutex_unlock(&pool->mutex);
                return pool->connections[i];
            }
        }
        // Wait for a connection to become available
        pthread_cond_wait(&pool->cond, &pool->mutex);
        LogTrace("condition");
    }
}

// Function to release a connection back to the pool
void sqlite_release_connection(SQLiteConnectionPool* pool, sqlite3* connection)
{
    pthread_mutex_lock(&pool->mutex);

    LogTrace("condition release");
    for (int i = 0; i < SQLITE_CONN_POOL_SIZE; i++) {
        if (pool->connections[i] == connection) {
            pool->in_use[i] = 0;
            break;
        }
    }

    LogTrace("sending condition");
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
}

// Function to destroy the connection pool
void sqlite_destroy_connection_pool(SQLiteConnectionPool* pool)
{
    pthread_mutex_lock(&pool->mutex);

    for (int i = 0; i < SQLITE_CONN_POOL_SIZE; i++) {
        if (pool->connections[i]) {
            sqlite3_close(pool->connections[i]);
            pool->connections[i] = NULL;
        }
    }

    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
}
