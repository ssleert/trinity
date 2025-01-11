#include "db.h"
#include "log.h"
#include "trinity.h"
#include "sqlite3.h"

static sqlite3* db;

static const char db_schema[] = STR(
  pragma journal_mode = WAL;
  pragma synchronous = normal;
  pragma journal_size_limit = 6144000;
  pragma temp_store = memory;
  pragma mmap_size = 30000000000;
  pragma page_size = 32768;


  CREATE TABLE IF NOT EXISTS users (
      id                INTEGER PRIMARY KEY AUTOINCREMENT,
      uuid              TEXT NOT NULL,
      nickname          TEXT UNIQUE NOT NULL,
      password_hash     TEXT NOT NULL,
      password_hash_pow TEXT NOT NULL
  );
);

int init_db(const char* filename) {
  // Open database connection
  int rc = sqlite3_open(filename, &db);
  if (rc) {
      LogErr("Can't open database: %s", sqlite3_errmsg(db));
      return EXIT_FAILURE;
  } else {
      LogTrace("Opened database successfully");
  }

  LogInfo("db_schema = '%s'", db_schema);

  char* err_msg = NULL;
  rc = sqlite3_exec(db, db_schema, NULL, NULL, &err_msg);
  if (rc != SQLITE_OK) {
      LogErr("Failed to initialize database schema: %s", err_msg);
      sqlite3_free(err_msg);
      return EXIT_FAILURE;
  }
  LogInfo("Database schema initialized successfully.");

  return 0;
}

int add_user_to_db(const User* user) {
  if (!user || !user->nickname || !user->password_hash || !user->uuid) {
      LogErr("Invalid user data provided.");
      return EXIT_FAILURE;
  }

  const char* sql = "INSERT INTO users (uuid, nickname, password_hash, password_hash_pow) VALUES (?, ?, ?, ?);";
  sqlite3_stmt* stmt;

  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
      LogErr("Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
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
      return EXIT_FAILURE;
  }

  LogInfo("User added to the database: UUID = %s, Nickname = %s", user->uuid, user->nickname);

  sqlite3_finalize(stmt);
  return EXIT_SUCCESS;
}
