#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

#define DEFAULT_DB_PATH "data/tasks.db"




typedef struct _task{
    int id;
    char *title;
    char *desc;
    int status;
    int due_date;
    int created_at;
    int updated_at;
    char *tag;
} Task;

typedef struct _TaskFormData{
    char title[256];
    char description[512];
    long due_date;
    char tag_name[31];
} TaskFormData;

typedef struct _MenuData{
    Task *task_list;
    int task_count;

    char **tag_list;
    int tag_count;

    char **filter_list;
    int filter_count;
} MenuData;


sqlite3* db_open(const char* filename);
int db_init_schema(sqlite3* db);
int load_tags(sqlite3 *db, char ***tags_list, int *tag_count);
void free_tags_list(char **tags_list, int tags_count);

int load_tasks_fillterd(sqlite3 *db, MenuData *data, const char *where_close);

void free_task_field(Task *task);
void free_tasks_list(MenuData *data);

void create_no_tasks_message(char *buffer, size_t buffer_size, const char *where_clause);
Task * task_placeholder(const char *title, const char *desc);

int db_tags_count(sqlite3 *db);

int is_tag_exist(sqlite3 *db, const char *new_tag);

int insert_new_task(sqlite3 *db, TaskFormData new_task);


#endif