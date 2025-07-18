// #define _XOPEN_SOURCE 700
#include <stdio.h>
// #include <time.h>
// #include <ncurses.h>

// // #include "todoist.h"
#include "database.h"


int main(){



    sqlite3 *db = db_open(DEFAULT_DB_PATH);

    if (db != NULL){
        printf("yay\n");
    }

    if (!db_init_schema(db)){
        printf("the table created\n");
    }

    sqlite3_exec();

    return 0;
}


