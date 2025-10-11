#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <menu.h>
#include <form.h>

#include "database.h"

#define SIDE_BAR_WIDTH 20

typedef struct _TasksPane{
    WINDOW *win;
    MENU *menu;
    ITEM **items;
    Tasks tasks_struct;
} TasksPane;

void init_tui(sqlite3 *db);
WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *win);
void update_time_top_bar(WINDOW *win, int y_pos, int x_pos);
void print_menu(WINDOW *menu_win, int highlight, char **list, int count);

void add_focusable_window(WINDOW *win, MENU *menu);


void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string);

void switch_focus(WINDOW *task_details_win);

void cleanup_menus();

void show_task_details(WINDOW *win ,Task *t);

WINDOW* create_top_bar();
void create_filter_menu(ITEM ***filter_items, MENU **filter_menu, WINDOW **filters_bar_win);
void create_tags_menu(sqlite3 *db, ITEM ***tags_items, MENU **tags_menu, WINDOW **tags_bar_win, char ***tags_list, int *tags_count);
void create_tasks_menu(sqlite3 *db, TasksPane *tasks_pane);
WINDOW* create_task_details_window();

void create_tasks_menu_test(TasksPane *tasks_pane);
void reload_tasks_menu(sqlite3 *db, TasksPane *tasks_pane, const char *where_clause);
void update_menu_highlighting();

void mark_selected_tag(int selected_index);
void mark_selected_filter(int selected_index);

// form:

WINDOW *create_form_window(void);
void destroy_form_window(WINDOW *form_win);
void show_add_task_form(sqlite3 *db);

char* show_tag_menu(WINDOW *parent_win, char **tags, int tag_count);
char *show_add_tag_win(WINDOW *parent_win, char **tags, int tag_count, sqlite3 *db);

#endif