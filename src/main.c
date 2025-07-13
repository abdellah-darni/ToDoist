#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <time.h>

// #include "file_io.h"
#include "todoist.h"
#include "database.h"

int main(){

    sqlite3 *db = db_open(DEFAULT_DB_PATH);
    

    if (!db_init_schema(db)){
        printf("the table created\n");
    }

    char *title = "title";
    char *desc = "desc 1";

    
    int rc = add_task(db,title, desc,1234567890);
    printf("\n rc = %d \n",rc);

    sqlite3_close(db);

    return 0;
}