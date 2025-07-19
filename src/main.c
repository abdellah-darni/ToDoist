
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>


// // // #include "todoist.h"
#include "database.h"
#include "tui.h"



int main(){

    sqlite3 *db = db_open(DEFAULT_DB_PATH);

    if (!db_init_schema(db)){
        printf("the table created\n");
    }

    int tags_count;
    char **tags_list = NULL;

    int state = load_tags(db, &tags_list, &tags_count);
    printf("state : %d\n", state);
    if (state != 0){
        printf("shitt !!");
        return 1;
    }

    for (int j = 0; j < tags_count; j++){
        printf("- %s\n",tags_list[j]);
    }

    // init_tui();
   

    return 0;
}