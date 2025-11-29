#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    free_tags_list(*tags_list, *tag_count);

    if(count == 0){
        free(temp_list);
        *tags_list = NULL;
        *tag_count = 0;

    } else {
        *tags_list = temp_list;
        *tag_count = count;
    }

    return 0;
}

void free_tags_list(char **tags_list, int tags_count){
    if (!tags_list) return;
    for (int i = 0; i < tags_count; i++){
        free(tags_list[i]);
    }
    free(tags_list);
}

int load_tasks_fillterd(sqlite3 *db, MenuData *data, const char *where_clause){

    if (!db || !data || !where_clause) return -1;

    sqlite3_stmt *stmt;
    int db_tasks_count = -1;
    int rc;

    char sql[512];

    snprintf(sql, sizeof(sql), "SELECT COUNT(DISTINCT t.id) "
                                "FROM tasks AS t "
                                "LEFT JOIN task_tags AS tt ON tt.task_id = t.id "
                                "LEFT JOIN tags AS tg ON tg.id = tt.tag_id "
                                "WHERE %s ;", where_clause);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        free_tasks_list(data);
        return -1;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        db_tasks_count = (int)sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    stmt = NULL;

    if (db_tasks_count == 0){
        
        char msg[256];
        create_no_tasks_message(msg, sizeof(msg), where_clause);

        Task *error_task = task_placeholder(msg, msg);

        free_tasks_list(data);
        data->task_list = error_task;
        data->task_count = 1;

        return 0;

    } else if (db_tasks_count == -1){
        free_tasks_list(data);
        return -1;
    }

    Task *new_list = calloc((size_t)db_tasks_count, sizeof(Task));

    if (!new_list){
        fprintf(stderr, "Failed to allocate memory for a tasks\n");
        free_tasks_list(data);
        return -1;
    }

    snprintf(sql, sizeof(sql), "SELECT "
                                    "t.id, "
                                    "t.title, "
                                    "t.description, "
                                    "t.status, "
                                    "t.due_date, "
                                    "t.created_at, "
                                    "t.updated_at, "
                                    "tg.name "
                                "  FROM tasks AS t "
                                "  LEFT JOIN task_tags AS tt ON tt.task_id = t.id "
                                "  LEFT JOIN tags AS tg ON tg.id = tt.tag_id "
                                "  WHERE %s "
                                "  GROUP BY t.id;", where_clause);
    
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        free(new_list);
        free_tasks_list(data);
        return -1;
    }

    int i = 0;

    while((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        Task tmp;
        tmp.id          = sqlite3_column_int(stmt, 0);
        
        const unsigned char *d = sqlite3_column_text(stmt, 2);
        tmp.desc        = d ? strdup((const char *)d) : NULL;
        tmp.status      = sqlite3_column_int(stmt, 3);
        tmp.due_date    = sqlite3_column_int(stmt, 4);
        tmp.created_at  = sqlite3_column_int(stmt, 5);
        tmp.updated_at  = sqlite3_column_int(stmt, 6);

        const unsigned char *tagtxt = sqlite3_column_text(stmt, 7);
        tmp.tag = tagtxt ? strdup((const char*)tagtxt) : NULL;

        // hundelling the title
        const unsigned char *t = sqlite3_column_text(stmt, 1);

        char *mark = tmp.status ? "[X] " :"[ ] ";
        size_t mark_len = strlen(mark);

        if (t){
            const char *title_str = (const char *)t;
            size_t title_len = strlen(title_str);

            char * the_title = (char *)malloc(mark_len + title_len + 1);
            if (the_title){
                strcpy(the_title, mark);
                strcat(the_title, title_str);

                tmp.title = the_title;
            }else{
                tmp.title = NULL;
            }
        }else{
            tmp.title = NULL;
        }

        new_list[i++] = tmp;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "Error iterating tasks: %s\n", sqlite3_errmsg(db));
        for (int j = 0; j < i; j++){
            free_task_field(&new_list[j]);
        }
        free(new_list);
        free_tasks_list(data);
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    stmt = NULL;

    free_tasks_list(data);
    data->task_list = new_list;
    data->task_count = db_tasks_count;

    return 0;
   
}

void free_task_field(Task *task){
    if (!task) return;

    free(task->title);
    task->title = NULL;
    free(task->desc);
    task->desc = NULL;
    free(task->tag);
    task->tag = NULL;
}

void free_tasks_list(MenuData *data){
    if (!data) return;

    if (data->task_list){
        for (int i = 0; i < data->task_count; i++){
            free_task_field(&data->task_list[i]);
        }
        free(data->task_list);
        data->task_list = NULL;
    }
    data->task_count = 0;
}

void create_no_tasks_message(char *buffer, size_t buffer_size, const char *where_clause) {

    if (!where_clause || strcmp(where_clause, "1=1") == 0 ){
        snprintf(buffer, buffer_size, "No tasks found");
    } else if (strstr(where_clause, "date")){
        if (strstr(where_clause, "date(due_date, 'unixepoch')=date('now')")){
            snprintf(buffer, buffer_size, "No tasks for today");
        } else if (strstr(where_clause, "date(due_date, 'unixepoch')=date('now','+1 day')")){
            snprintf(buffer, buffer_size, "No tasks for tomorow");
        } else {
            snprintf(buffer, buffer_size, "No over-due tasks");
        }
    } else if (strstr(where_clause, "t.status=1")){
        snprintf(buffer, buffer_size, "No completed tasks");
    } else if (strstr(where_clause, "tg.name")){
        char *tag_start = strstr(where_clause, "'");
        if (tag_start){
            tag_start++;
            char *tag_end = strchr(tag_start, '\'');
            if (tag_end){
                int tag_len = tag_end - tag_start;
                if (tag_len < 50){
                    snprintf(buffer, buffer_size, "No tasks with tag %.*s", tag_len, tag_start);
                } else {
                    snprintf(buffer, buffer_size, "No tasks with the selected tag");
                }
            } else {
                snprintf(buffer, buffer_size, "No tasks with the selected tag");
            }
        } else {
            snprintf(buffer, buffer_size, "No tasks with the selected tag");
        }
    } else {
        snprintf(buffer, buffer_size, "No tasks match current filter");
    }
}

Task * task_placeholder(const char *title, const char *desc){

    Task *error_task = calloc(1, sizeof(Task));

    if (!error_task){
        return NULL;
    }

    error_task->id = -1;
    error_task->title = title ? strdup(title) : NULL;
    error_task->desc = desc ? strdup(desc) : NULL;
    error_task->status = 0;
    error_task->created_at = 0;
    error_task->updated_at = 0;
    error_task->due_date = 0;
    error_task->tag = NULL;

    return error_task;
}

int is_tag_exist(sqlite3 *db, const char *new_tag){
    if (!db || !new_tag) return -1;

    sqlite3_stmt *stmt;
    int rc;

    char sql[512];

    snprintf(sql, sizeof(sql), "SELECT EXISTS ( "
                                    "SELECT 1 "
                                    "FROM tags "
                                    "WHERE name = '%s');", new_tag);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return -1;
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        int tmp = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        return tmp;
    }
    sqlite3_finalize(stmt);
    return -1;
}

int db_tags_count(sqlite3 *db){
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(id) FROM tags;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    sqlite3_int64 count = -1;
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return (int)count;
}

// CRUD 

int insert_new_task(sqlite3 *db, TaskFormData new_task){
    if (!db) return 1;

    sqlite3_int64 tag_id = -1;
    sqlite3_int64 new_added_task_id = -1;

    sqlite3_stmt *stmt = NULL;
    const char *insert_task_sql = "INSERT INTO tasks (title, description, due_date) VALUES(?, ?, ?) RETURNING id;";
    const char *insert_tag_sql = "INSERT INTO tags (name) VALUES(?) RETURNING id;";
    const char *tag_id_sql = "SELECT id from tags where name = ?;";
    const char *task_tag_association_sql = "INSERT INTO task_tags (task_id, tag_id) VALUES(?, ?);";
    int rc;

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return 1;
    }


    // add the new task
    rc = sqlite3_prepare_v2(db, insert_task_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        return 1;
    }

    sqlite3_bind_text(stmt, 1, new_task.title, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, new_task.description, -1, SQLITE_TRANSIENT);
    if(new_task.due_date == -1){
        sqlite3_bind_null(stmt, 3);
    } else {
        sqlite3_bind_int64(stmt, 3, (sqlite3_int64)new_task.due_date);
    }

    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW){
        new_added_task_id = sqlite3_column_int64(stmt, 0);
    } else {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_finalize(stmt);
        return 1;
    }

    if ((rc = sqlite3_step(stmt))!= SQLITE_DONE){
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
        sqlite3_finalize(stmt);
        return 1;
    }

    sqlite3_finalize(stmt);

    // tag stuff, if there is a tag try to retrive the id if we get a row that mean that the tag already exist in the table so just retrive the id 
    // if we didn't get a row that mean the tag doesn't exist in the table/db so insert it and retrive the id
    if (strcmp(new_task.tag_name, "None") != 0){
        rc = sqlite3_prepare_v2(db, tag_id_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            return 1;
        }

        sqlite3_bind_text(stmt, 1, new_task.tag_name, -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_DONE){
            sqlite3_finalize(stmt);

            rc = sqlite3_prepare_v2(db, insert_tag_sql, -1, &stmt, NULL);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
                sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
                return 1;
            }
            sqlite3_bind_text(stmt, 1, new_task.tag_name, -1, SQLITE_TRANSIENT);
            if ((rc = sqlite3_step(stmt))== SQLITE_ROW){
                tag_id = sqlite3_column_int64(stmt, 0);
            } else {
                fprintf(stderr, " 511: SQL error: %s\n", sqlite3_errmsg(db));
                sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
                sqlite3_finalize(stmt);
                return 1;
            }
        } else if (rc == SQLITE_ROW){
            tag_id = sqlite3_column_int64(stmt, 0);
        }

        if ((rc = sqlite3_step(stmt))!= SQLITE_DONE){
            fprintf(stderr, "521: SQL error: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            sqlite3_finalize(stmt);
            return 1;
        }

        sqlite3_finalize(stmt);

        // the assocation between the task and the tag
        rc = sqlite3_prepare_v2(db, task_tag_association_sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            return 1;
        }

        sqlite3_bind_int64(stmt, 1, new_added_task_id);
        sqlite3_bind_int64(stmt, 2, tag_id);

        if ((rc = sqlite3_step(stmt))!= SQLITE_DONE){
            fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            sqlite3_finalize(stmt);
            return 1;
        }
        sqlite3_finalize(stmt);
    }

    rc = sqlite3_exec(db, "COMMIT TRANSACTION;", 0, 0, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to commit transaction: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    return 0;

}

int update_task(sqlite3 *db, Task *task){

    if (!db){
        return 1;
    }

    sqlite3_stmt *stmt = NULL;
    int rc;
    const char *sql = "UPDATE tasks SET title = ?, description = ?, status = ?, due_date = ?, updated_at = ? WHERE id = ?;";

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to BEGIN TRANSACTION [update_task]: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}

    const char *clean_title = task->title;
    if (task->title != NULL && task->title[0] == '[' && strlen(task->title) > 4) {
        clean_title = task->title + 4;
    }

    sqlite3_bind_text(stmt, 1, clean_title, -1, SQLITE_STATIC);

    if (task->desc != NULL && strlen(task->desc) > 0) {
        sqlite3_bind_text(stmt, 2, task->desc, -1, SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 2);
    }
    sqlite3_bind_int(stmt, 3, task->status);

    if (task->due_date > 0) {
        sqlite3_bind_int64(stmt, 4, (sqlite3_int64)task->due_date);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    time_t now = time(NULL);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)now);

    sqlite3_bind_int(stmt, 7, task->id);

    // char *debug_query = sqlite3_expanded_sql(stmt);
    // if (debug_query) {
    //     fprintf(stderr, "DEBUG SQL: %s\n", debug_query);
    //     sqlite3_free(debug_query); 
    // }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE){goto rollback;}
    sqlite3_finalize(stmt);
    stmt = NULL;
    
    // delete the task-tag assocation
    const char *del_tag_sql = "DELETE FROM task_tags WHERE task_id = ?;";
    rc = sqlite3_prepare_v2(db, del_tag_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}
    sqlite3_bind_int(stmt, 1, task->id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE){goto rollback;}
    sqlite3_finalize(stmt);
    stmt = NULL;

    if (strcmp(task->tag, "None") == 0){goto commit;}

    const char *insert_tag_sql = "INSERT OR IGNORE INTO tags (name) VALUES (?);";
    rc = sqlite3_prepare_v2(db, insert_tag_sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}
    sqlite3_bind_text(stmt, 1, task->tag, -1,SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE){goto rollback;}
    sqlite3_finalize(stmt);
    stmt = NULL;

    sqlite3_int64 tag_id = -1;
    const char *get_tag_id = "SELECT id FROM tags WHERE name = ?;";
    rc = sqlite3_prepare_v2(db, get_tag_id, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}
    sqlite3_bind_text(stmt, 1, task->tag, -1,SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW){goto rollback;}
    tag_id = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    stmt = NULL;

    if (tag_id == -1){goto commit;}
    const char *link = "INSERT INTO task_tags (task_id, tag_id) VALUES (?, ?);";
    rc = sqlite3_prepare_v2(db, link, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}
    sqlite3_bind_int64(stmt, 1, task->id);
    sqlite3_bind_int64(stmt, 2, tag_id);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE){goto rollback;}
    sqlite3_finalize(stmt);
    stmt = NULL;

commit:
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    return 0;

rollback:
    if(stmt) sqlite3_finalize(stmt);
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    return 1;
}

int delete_task(sqlite3 *db, Task *task){

    if (!db){
        return 0;
    }

    const char *sql = "DELETE FROM tasks WHERE id = ?;";

    sqlite3_stmt *stmt = NULL;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){
        fprintf(stderr, "Failed to prepare statment [update_task]: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_int(stmt, 1, task->id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

int insert_new_tag(sqlite3 *db, const char *new_tag){

    if (!db) return 1;

    const char *sql = "INSERT INTO tags (name) VALUES (?);";

    sqlite3_stmt *stmt = NULL;
    int rc;

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK){goto rollback;}

    sqlite3_bind_text(stmt, 1, new_tag, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE){goto rollback;}
    sqlite3_finalize(stmt);

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    return 0;

rollback:
    if(stmt) sqlite3_finalize(stmt);
    sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    return 1;
}