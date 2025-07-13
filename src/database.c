#include <stdio.h>
#include <stdlib.h>

#include "database.h"

sqlite3* db_open(const char* filename){
    sqlite3 *db;
    
    int rc = sqlite3_open(filename, &db);

    if (rc != SQLITE_OK){
        fprintf(stderr, "Cannot open database %s\n", sqlite3_errmsg(db));
        return NULL;
    }
    printf("the db is secesfuly buildet\n");
    return db;
}

int db_init_schema(sqlite3* db){
    const char *create_table_sql = 
    "CREATE TABLE IF NOT EXISTS tasks ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "title TEXT NOT NULL,"
    "status INTEGER DEFAULT 0,"
    "due_date INEGER,"
    "created_at INTEGER DEFAULT (strftime('%s', 'now')),"
    "updated_at INTEGER DEFAULT (strftime('%s', 'now'))"
    ");";

    char *err_msg = NULL;
    int rc = sqlite3_exec(db, create_table_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    return 0;
}




