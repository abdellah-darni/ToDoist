
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>



#include "database.h"
#include "tui.h"

// void free_tasks(Tasks *tasks) {
//     for (int i = 0; i < tasks->task_count; i++) {
//         free(tasks->task_list[i].title);
//         free(tasks->task_list[i].desc);
//         free(tasks->task_list[i].tag);
//     }
//     free(tasks->task_list);
//     tasks->task_list = NULL;
//     tasks->task_count = 0;
// }

int main(){

    sqlite3 *db = db_open(DEFAULT_DB_PATH);
    db_init_schema(db);

    // Tasks all_tasks;
    // all_tasks.task_count = 0;
    // all_tasks.task_list = NULL;

    // if (!db_init_schema(db)){
    //     printf("the table created\n");
    // }

    // char *where_close = "1 = 1";

    // load_tasks_fillterd(db, &all_tasks, where_close);

    // // load_tasks(db, &all_tasks);

    // for(int i = 0; i < all_tasks.task_count; i++){
    //     printf("id: %d\ttitle: %s\tdesc: %s\ttags: %s\n",all_tasks.task_list[i].id, all_tasks.task_list[i].title, all_tasks.task_list[i].desc, all_tasks.task_list[i].tag);
    // }

    // free_tasks(&all_tasks);

    init_tui(db);
   

    return 0;
}