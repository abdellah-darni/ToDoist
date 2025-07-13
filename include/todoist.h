#ifndef TODOIST_H
#define TODOIST_H

#include <time.h>

#define MAX_TITLE_LEN 256
#define MAX_DESC_LEN 1024

typedef enum
{
    TASK_PENDING,
    TASK_COMPLETED,
    TASK_OVERDUE
} TaskStatus;

typedef struct
{
    int id;
    char title[MAX_TITLE_LEN];
    char description[MAX_DESC_LEN];
    TaskStatus status;
    time_t due_date;
    time_t created_at;
    time_t updated_at;
} Task;





#endif