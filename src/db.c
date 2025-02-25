#include "db.h"
#include "log.h"
#include "sqlite_connection_pool.h"
#include "trinity.h"
#include <string.h>

static SQLiteConnectionPool conn_pool;

static const char db_schema[] = STR(
    pragma journal_mode = WAL;
    pragma synchronous = normal;
    pragma journal_size_limit = 6144000;
    pragma temp_store = memory;
    pragma mmap_size = 30000000000;
    pragma page_size = 32768;

    CREATE TABLE IF NOT EXISTS users(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        uuid TEXT NOT NULL,
        nickname TEXT UNIQUE NOT NULL,
        password_hash TEXT NOT NULL,
        password_hash_pow TEXT NOT NULL);

    CREATE TABLE IF NOT EXISTS sessions(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        session_key TEXT NOT NULL,
        user_id INTEGER NOT NULL,

        FOREIGN KEY(user_id) REFERENCES users(id));

    CREATE TABLE IF NOT EXISTS messages(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        deleted_at BIGINTEGER,
        created_at BIGINTEGER NOT NULL,
        updated_at BIGINTEGER NOT NULL,
        uuid TEXT NOT NULL,
        sender_id INTEGER NOT NULL,
        receiver_id INTEGER NOT NULL,
        data TEXT NOT NULL,

        FOREIGN KEY(sender_id) REFERENCES users(id),
        FOREIGN KEY(receiver_id) REFERENCES users(id)););

int init_db(void)
{
    // Open database connection
    int rc = init_sqlite_connection_pool(&conn_pool, "main.db");
    if (rc) {
        LogErr("Can't open sqlite3 connection pool");
        return EXIT_FAILURE;
    } else {
        LogTrace("Opened database successfully");
    }

    LogInfo("db_schema = '%s'", db_schema);

    sqlite3* db = sqlite_get_connection(&conn_pool);

    char* err_msg = NULL;
    rc = sqlite3_exec(db, db_schema, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LogErr("Failed to initialize database schema: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }
    LogInfo("Database schema initialized successfully.");

    sqlite_release_connection(&conn_pool, db);

    return 0;
}

int add_user_to_db(const User* user)
{
    if (!user || !user->nickname || !user->password_hash || !user->uuid) {
        LogErr("Invalid user data provided.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "INSERT INTO users (uuid, nickname, password_hash, password_hash_pow) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_bind_text(stmt, 1, user->uuid, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user->nickname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user->password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user->password_hash_pow, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    LogInfo("User added to the database: UUID = %s, Nickname = %s", user->uuid, user->nickname);

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_SUCCESS;
}

int get_user_password_hash_and_pow_and_id_by_nickname_from_db(const char* nickname, char* password_hash, char* password_hash_pow, int* id)
{
    if (!nickname || !password_hash || !password_hash_pow) {
        LogErr("Invalid input parameters.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "SELECT password_hash, password_hash_pow, id FROM users WHERE nickname = ?;";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        sqlite_release_connection(&conn_pool, db);
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    // Bind the nickname parameter
    sqlite3_bind_text(stmt, 1, nickname, -1, SQLITE_STATIC);

    // Execute the statement and fetch the result
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Copy the results to the provided buffers
        const char* hash = (const char*)sqlite3_column_text(stmt, 0);
        const char* hash_pow = (const char*)sqlite3_column_text(stmt, 1);
        int user_id = sqlite3_column_int(stmt, 2);

        *id = user_id;

        if (hash) {
            strcpy(password_hash, hash);
        } else {
            LogErr("Password hash is NULL for nickname: %s", nickname);
            sqlite3_finalize(stmt);
            sqlite_release_connection(&conn_pool, db);
            return EXIT_FAILURE;
        }

        if (hash_pow) {
            strcpy(password_hash_pow, hash_pow);
        } else {
            LogErr("Password hash pow is NULL for nickname: %s", nickname);
            sqlite3_finalize(stmt);
            sqlite_release_connection(&conn_pool, db);
            return EXIT_FAILURE;
        }

        LogInfo("Password hash and pow retrieved for nickname: %s", nickname);
    } else if (rc == SQLITE_DONE) {
        LogWarn("No user found with nickname: %s", nickname);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    } else {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Finalize the statement
    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_SUCCESS;
}

int get_user_id_by_uuid(const char* uuid, int* user_id)
{
    if (!uuid || !user_id) {
        LogErr("Invalid parameters provided.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "SELECT id FROM users WHERE uuid = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *user_id = sqlite3_column_int(stmt, 0);
        LogInfo("Found user ID for UUID %s: %d", uuid, *user_id);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_SUCCESS;
    } else if (rc == SQLITE_DONE) {
        LogErr("No user found with UUID: %s", uuid);
    } else {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_FAILURE;
}

int get_user_uuid_by_id(int user_id, char* uuid)
{
    if (!uuid) {
        LogErr("Invalid parameters provided.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "SELECT uuid FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const char* retrieved_uuid = (const char*)sqlite3_column_text(stmt, 0);
        if (retrieved_uuid) {
            strcpy(uuid, retrieved_uuid); // Directly copy the UUID
            sqlite3_finalize(stmt);
            sqlite_release_connection(&conn_pool, db);
            LogInfo("Found UUID for user ID %d: %s", user_id, uuid);
            return EXIT_SUCCESS;
        }
    } else if (rc == SQLITE_DONE) {
        LogErr("No user found with ID: %d", user_id);
    } else {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_FAILURE;
}

int add_session_to_db(const Session* session)
{
    if (!session || !session->session_key || session->user_id <= 0) {
        LogErr("Invalid session data provided.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    // SQL statement to insert a session into the sessions table
    const char* sql = "INSERT INTO sessions (session_key, user_id) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        return EXIT_FAILURE;
    }

    // Bind values to the prepared statement
    sqlite3_bind_text(stmt, 1, session->session_key, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, session->user_id);

    // Execute the SQL statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    LogInfo("Session added to the database: Session Key = %s, User ID = %d", session->session_key, session->user_id);

    // Finalize the statement to release resources
    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_SUCCESS;
}

int get_sessions_id_by_user_id(int user_id, Session** sessions, size_t* sessions_len)
{
    if (!sessions || !sessions_len) {
        LogErr("Invalid arguments: sessions or sessions_len is NULL.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    *sessions = NULL;
    *sessions_len = 0;

    const char* sql = "SELECT session_key, id FROM sessions WHERE user_id = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    size_t capacity = 10; // Initial capacity for the sessions array
    *sessions = malloc(capacity * sizeof(Session));
    if (!*sessions) {
        LogErr("Memory allocation failed for sessions array.");
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Fetch all the session keys for the given user_id
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        // If we've reached the capacity limit, expand the array
        if (*sessions_len == capacity) {
            capacity *= 2;
            *sessions = realloc(*sessions, capacity * sizeof(Session));
            if (!*sessions) {
                LogErr("Memory reallocation failed for sessions array.");
                sqlite3_finalize(stmt);
                sqlite_release_connection(&conn_pool, db);
                return EXIT_FAILURE;
            }
        }

        // Store the session_id and session_key in the array
        Session* current_session = &(*sessions)[*sessions_len];
        current_session->user_id = user_id;

        const char* session_key = (const char*)sqlite3_column_text(stmt, 0);
        current_session->session_key = strdup(session_key); // Duplicate the session key string

        (*sessions_len)++;
    }

    if (rc != SQLITE_DONE) {
        LogErr("Failed to fetch session data: %s", sqlite3_errmsg(db));
        // Free allocated memory
        for (size_t i = 0; i < *sessions_len; i++) {
            free((*sessions)[i].session_key);
        }
        free(*sessions);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_SUCCESS;
}

int add_message_to_db(const Message* message)
{
    if (!message || !message->uuid || !message->data) {
        LogErr("Invalid input parameters.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "INSERT INTO messages (created_at, updated_at, uuid, sender_id, receiver_id, data) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Bind the parameters to the SQL statement
    sqlite3_bind_int64(stmt, 1, message->created_at);
    sqlite3_bind_int64(stmt, 2, message->updated_at);
    sqlite3_bind_text(stmt, 3, message->uuid, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, message->sender_id);
    sqlite3_bind_int(stmt, 5, message->receiver_id);
    sqlite3_bind_text(stmt, 6, message->data, -1, SQLITE_STATIC);

    // Execute the statement
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    LogInfo("Message added to the database successfully.");

    // Finalize the statement
    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_SUCCESS;
}

int get_user_id_by_session_key(const char* session_key, int* user_id)
{
    if (!session_key || !user_id) {
        LogErr("Invalid input: session_key or user_id pointer is NULL.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    const char* sql = "SELECT user_id FROM sessions WHERE session_key = ?;";
    sqlite3_stmt* stmt;

    // Prepare the SQL statement
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Bind the session_key to the SQL statement
    sqlite3_bind_text(stmt, 1, session_key, -1, SQLITE_STATIC);

    // Execute the SQL statement
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Retrieve the user_id from the result
        *user_id = sqlite3_column_int(stmt, 0);
        LogInfo("User ID retrieved for session key: %s -> User ID: %d", session_key, *user_id);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_SUCCESS;
    } else if (rc == SQLITE_DONE) {
        // No matching session key found
        LogWarn("No user found for session key: %s", session_key);
    } else {
        // An error occurred
        LogErr("Failed to execute SQL statement: %s", sqlite3_errmsg(db));
    }

    // Finalize the SQL statement
    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);
    return EXIT_FAILURE;
}

int get_all_senders_uuid_and_nicknames_by_user_id_from_session_key(
    char* session_key, SenderUuidAndNickname** senders, size_t* senders_len)
{
    if (!session_key || !senders || !senders_len) {
        LogErr("Invalid input: session_key, senders, or senders_len is NULL.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    // Find the user_id associated with the given session_key
    int user_id = 0;
    if (get_user_id_by_session_key(session_key, &user_id) != EXIT_SUCCESS) {
        LogErr("Failed to retrieve user ID for session key: %s", session_key);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    const char* sql =
        "SELECT DISTINCT u.uuid, u.nickname "
        "FROM messages m "
        "JOIN users u ON m.sender_id = u.id "
        "WHERE m.receiver_id = ?;";

    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Bind the user_id parameter
    sqlite3_bind_int(stmt, 1, user_id);

    *senders = NULL;
    *senders_len = 0;

    size_t capacity = 10; // Initial capacity for the senders array
    *senders = malloc(capacity * sizeof(SenderUuidAndNickname));
    if (!*senders) {
        LogErr("Memory allocation failed for senders array.");
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*senders_len == capacity) {
            capacity *= 2;
            *senders = realloc(*senders, capacity * sizeof(SenderUuidAndNickname));
            if (!*senders) {
                LogErr("Memory reallocation failed for senders array.");
                sqlite3_finalize(stmt);
                sqlite_release_connection(&conn_pool, db);
                return EXIT_FAILURE;
            }
        }

        SenderUuidAndNickname* current_sender = &(*senders)[*senders_len];

        const char* uuid = (const char*)sqlite3_column_text(stmt, 0);
        const char* nickname = (const char*)sqlite3_column_text(stmt, 1);

        current_sender->uuid = strdup(uuid);
        current_sender->nickname = strdup(nickname);

        if (!current_sender->uuid || !current_sender->nickname) {
            LogErr("Memory allocation failed for sender UUID or nickname.");
            for (size_t i = 0; i < *senders_len; i++) {
                free((*senders)[i].uuid);
                free((*senders)[i].nickname);
            }
            free(*senders);
            sqlite3_finalize(stmt);
            sqlite_release_connection(&conn_pool, db);
            return EXIT_FAILURE;
        }

        (*senders_len)++;
    }

    if (rc != SQLITE_DONE) {
        LogErr("Failed to fetch sender data: %s", sqlite3_errmsg(db));
        for (size_t i = 0; i < *senders_len; i++) {
            free((*senders)[i].uuid);
            free((*senders)[i].nickname);
        }
        free(*senders);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);

    LogInfo("Retrieved %zu senders for user ID %d.", *senders_len, user_id);
    return EXIT_SUCCESS;
}

int get_messages_by_reciever_user_id_from_session_key_and_sender_user_uuid(
    char* session_key, char* sender_uuid, int offset, int limit,
    MessageWithTimeAndData** msgs, size_t* msgs_len) {

    if (!session_key || !sender_uuid || !msgs || !msgs_len) {
        LogErr("Invalid input: session_key, sender_uuid, msgs, or msgs_len is NULL.");
        return EXIT_FAILURE;
    }

    sqlite3* db = sqlite_get_connection(&conn_pool);

    // Find the user_id associated with the given session_key
    int receiver_user_id = 0;
    if (get_user_id_by_session_key(session_key, &receiver_user_id) != EXIT_SUCCESS) {
        LogErr("Failed to retrieve user ID for session key: %s", session_key);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Find the sender_id associated with the sender_uuid
    int sender_user_id = 0;
    if (get_user_id_by_uuid(sender_uuid, &sender_user_id) != EXIT_SUCCESS) {
        LogErr("Failed to retrieve user ID for sender UUID: %s", sender_uuid);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    const char* sql =
        "SELECT data, created_at, updated_at "
        "FROM messages "
        "WHERE receiver_id = ? AND sender_id = ? AND deleted_at IS NULL "
        "ORDER BY created_at ASC "
        "LIMIT ? OFFSET ?;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    // Bind parameters
    sqlite3_bind_int(stmt, 1, receiver_user_id);
    sqlite3_bind_int(stmt, 2, sender_user_id);
    sqlite3_bind_int(stmt, 3, limit);
    sqlite3_bind_int(stmt, 4, offset);

    *msgs = NULL;
    *msgs_len = 0;

    size_t capacity = 10; // Initial capacity for the messages array
    *msgs = malloc(capacity * sizeof(MessageWithTimeAndData));
    if (!*msgs) {
        LogErr("Memory allocation failed for messages array.");
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (*msgs_len == capacity) {
            capacity *= 2;
            *msgs = realloc(*msgs, capacity * sizeof(MessageWithTimeAndData));
            if (!*msgs) {
                LogErr("Memory reallocation failed for messages array.");
                sqlite3_finalize(stmt);
                sqlite_release_connection(&conn_pool, db);
                return EXIT_FAILURE;
            }
        }

        MessageWithTimeAndData* current_msg = &(*msgs)[*msgs_len];

        const char* data = (const char*)sqlite3_column_text(stmt, 0);
        time_t created_at = sqlite3_column_int64(stmt, 1);
        time_t updated_at = sqlite3_column_int64(stmt, 2);

        current_msg->data = strdup(data);
        current_msg->created_at = created_at;
        current_msg->updated_at = updated_at;

        if (!current_msg->data) {
            LogErr("Memory allocation failed for message data.");
            for (size_t i = 0; i < *msgs_len; i++) {
                free((*msgs)[i].data);
            }
            free(*msgs);
            sqlite3_finalize(stmt);
            sqlite_release_connection(&conn_pool, db);
            return EXIT_FAILURE;
        }

        (*msgs_len)++;
    }

    if (rc != SQLITE_DONE) {
        LogErr("Failed to fetch message data: %s", sqlite3_errmsg(db));
        for (size_t i = 0; i < *msgs_len; i++) {
            free((*msgs)[i].data);
        }
        free(*msgs);
        sqlite3_finalize(stmt);
        sqlite_release_connection(&conn_pool, db);
        return EXIT_FAILURE;
    }

    sqlite3_finalize(stmt);
    sqlite_release_connection(&conn_pool, db);

    LogInfo("Retrieved %zu messages for receiver ID %d and sender ID %d.", *msgs_len, receiver_user_id, sender_user_id);
    return EXIT_SUCCESS;
}

