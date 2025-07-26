#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int load_tags(sqlite3 *db, char ***tags_list, int *tag_count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT name FROM tags ORDER BY name;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    int count = 0;
    int capacity = 10;
    char **temp_list = malloc(capacity * sizeof(char *));
    if (temp_list == NULL) {
        fprintf(stderr, "Failed to allocate memory for tags list\n");
        sqlite3_finalize(stmt);
        return -1;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        if (count >= capacity) {
            capacity *= 2;
            char **realloc_list = realloc(temp_list, capacity * sizeof(char *));
            if (realloc_list == NULL) {
                fprintf(stderr, "Failed to reallocate memory for tags list\n");

                for (int i = 0; i < count; i++) {
                    free(temp_list[i]);
                }
                free(temp_list);
                sqlite3_finalize(stmt);
                return -1;
            }
            temp_list = realloc_list;
        }

        const char *tag_text = (const char *)sqlite3_column_text(stmt, 0);
        if (tag_text) {
            temp_list[count] = malloc(strlen(tag_text) + 1);
            if (temp_list[count] == NULL) {
                fprintf(stderr, "Failed to allocate memory for a tag\n");

                 for (int i = 0; i < count; i++) {
                    free(temp_list[i]);
                }
                free(temp_list);
                sqlite3_finalize(stmt);
                return -1;
            }
            strcpy(temp_list[count], tag_text);
            count++;
        }
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    *tags_list = temp_list;
    *tag_count = count;

    return 0;
}


int load_tasks(sqlite3 *db, Tasks *tasks){
    sqlite3_stmt *stmt;
    int db_tasks_count = 0;
    int rc;

    const char *sql_count = "SELECT COUNT(*) FROM tasks;";

    rc = sqlite3_prepare_v2(db, sql_count, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        db_tasks_count = (int)sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

    Task *tmp_list = realloc(tasks->task_list, db_tasks_count * sizeof(Task));
    if (!tmp_list){
        fprintf(stderr, "Failed to allocate memory for a tasks\n");
        return -1;
    }

    tasks->task_list = tmp_list;

    const char *sql_tasks = "SELECT t.id, t.title, t.description, t.status, "
                        "       t.due_date, t.created_at, t.updated_at, tg.name "
                        "  FROM tasks AS t "
                        "  LEFT JOIN task_tags AS tt ON tt.task_id = t.id "
                        "  LEFT JOIN tags       AS tg ON tg.id      = tt.tag_id"
                        "  GROUP BY t.id;";
    
    rc = sqlite3_prepare_v2(db, sql_tasks, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    int i = 0;

    while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        Task tmp;
        tmp.id          = sqlite3_column_int(stmt, 0);
        tmp.title       = strdup((char *)sqlite3_column_text(stmt, 1));
        tmp.desc        = strdup((char *)sqlite3_column_text(stmt, 2));
        tmp.status      = sqlite3_column_int(stmt, 3);
        tmp.due_date    = sqlite3_column_int(stmt, 4);
        tmp.created_at  = sqlite3_column_int(stmt, 5);
        tmp.updated_at  = sqlite3_column_int(stmt, 6);

        const unsigned char *tagtxt = sqlite3_column_text(stmt, 7);
        tmp.tag = tagtxt ? strdup((const char*)tagtxt) : NULL;

        tasks->task_list[i++] = tmp;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error iterating tasks: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    tasks->task_count = db_tasks_count;

    return 0;
}