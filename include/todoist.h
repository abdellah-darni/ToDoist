#ifndef TODOIST_H
#define TODOIST_H

#include <time.h>
#include <sqlite3.h>

#define MAX_TITLE_LEN 256
#define MAX_DESC_LEN 1024

typedef enum
{
    TASK_PENDING,
    TASK_COMPLETED,
    TASK_OVERDUE
} TaskStatus;



int add_task(sqlite3 *db, char *title, char *descreption, int due_date);
int update_task(sqlite3 *db, int id, char *title, char *descreption, int due_date);
int delete_task();
int set_task_status();


#endif