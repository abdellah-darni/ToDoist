#include <stdlib.h>
#include <stdio.h>

#include "todoist.h"

int add_task(sqlite3 *db, char *title, char *descreption, int due_date){

    if (!db || !title) return -1;

    char *sql = NULL;
    if (due_date > 0){
        sql = "INSERT INTO tasks (title, description, due_date) VALUES (?, ?, ?);";
    }else if (due_date == 0){
        sql = "INSERT INTO tasks (title, description) VALUES (?, ?);";
    }


    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to prepare statment [add_task]: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, descreption ? descreption : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, due_date);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE){
        fprintf(stderr, "Failed to insert task: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}
