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
    const char *create_tables_sql[] = {
      "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "status INTEGER DEFAULT 0,"
        "due_date INTEGER,"
        "created_at INTEGER DEFAULT (strftime('%s','now')),"
        "updated_at INTEGER DEFAULT (strftime('%s','now'))"
      ");",
      "CREATE TABLE IF NOT EXISTS tags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE"
      ");",
      "CREATE TABLE IF NOT EXISTS task_tags ("
        "task_id INTEGER NOT NULL,"
        "tag_id  INTEGER NOT NULL,"
        "PRIMARY KEY (task_id, tag_id),"
        "FOREIGN KEY (task_id) REFERENCES tasks(id) ON DELETE CASCADE,"
        "FOREIGN KEY (tag_id)  REFERENCES tags(id)  ON DELETE CASCADE"
      ");"
    };

    char *err_msg = NULL;
    for (int i = 0; i < 3; i++ ){
        int rc = sqlite3_exec(db, create_tables_sql[i], NULL, NULL, &err_msg);
        if (rc != SQLITE_OK){
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
            return -1;
        }
    }
    return 0;
}
