
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>



// #include "todoist.h"
#include "database.h"
#include "tui.h"



int main(){

    sqlite3 *db = db_open(DEFAULT_DB_PATH);

    Tasks all_tasks;
    all_tasks.task_count = 0;
    all_tasks.task_list = NULL;

    if (!db_init_schema(db)){
        printf("the table created\n");
    }

    load_tasks(db, &all_tasks);

    for(int i = 0; i < all_tasks.task_count; i++){
        printf("id: %d\ttitle: %s\tdesc: %s\ttags: %s\n",all_tasks.task_list[i].id, all_tasks.task_list[i].title, all_tasks.task_list[i].desc, all_tasks.task_list[i].tag);
    }


    init_tui(db);
   

    return 0;
}