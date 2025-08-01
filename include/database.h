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

typedef struct _tasks{
    Task *task_list;
    int task_count;
} Tasks;




sqlite3* db_open(const char* filename);
int db_init_schema(sqlite3* db);
int load_tags(sqlite3 *db, char ***tags_list, int *tag_count);

int load_tasks(sqlite3 *db, Tasks *tasks);

int load_tasks_fillterd(sqlite3 *db, Tasks *tasks, const char *where_close);



#endif