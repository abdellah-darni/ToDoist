#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

#define DEFAULT_DB_PATH "data/tasks.db"




sqlite3* db_open(const char* filename);
int db_init_schema(sqlite3* db);
int load_tags(sqlite3 *db, char ***tags_list, int *tag_count);


#endif