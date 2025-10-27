#ifndef TUI_H
#define TUI_H

#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <panel.h>

#include "database.h"

#define SIDE_BAR_WIDTH 20

typedef enum _MenuType{
    MENU_TYPE_FILTER,
    MENU_TYPE_TAG,
    MENU_TYPE_TASK
} MenuType;

typedef enum _ViewMode{
    VIEW_MODE_ALL,
    VIEW_MODE_TODAY,
    VIEW_MODE_TOMORROW,
    VIEW_MODE_OVERDUE,
    VIEW_MODE_COMPLETED,
    VIEW_MODE_BY_TAG
} ViewMode;

// typedef struct _MenuData{
//     Task *task_list;
//     int task_count;

//     char **tag_list;
//     int tag_count;

//     char **filter_list;
//     int filter_count;
// } MenuData;

typedef struct _FocusableMenu {
    PANEL *panel;
    WINDOW *win;
    WINDOW *subwin;
    MENU *menu;
    ITEM **items;

    MenuType type;
    char* title;

    int is_focused;
    int selected_index;

    MenuData data;

    int height, width;
    int starty, startx;
} FocusableMenu;

typedef struct _AppState{
    ViewMode mode;
    char *current_filter; // sql where clause

    int active_filter_index; // wich filter is active -1 if none
    int active_tag_index; // which tag is active -1 if none

    char *selected_tag_name; // 

    FocusableMenu **menus;
    int menu_count;
    int current_focus_idx;

    sqlite3 *db;
    
    PANEL *top_bar_panel;
    WINDOW *top_bar_win;
    PANEL *details_panel;
    WINDOW *details_win;
} AppState;

void init_app_state(sqlite3 *db);
void init_tui(sqlite3 *db);
FocusableMenu *creat_focusable_menu(MenuType type, char *title, int height, int width, int starty, int startx);
void free_menu_data(FocusableMenu *menu);
void register_menu(FocusableMenu *menu);
FocusableMenu *get_menu_by_type(MenuType type);
FocusableMenu *get_focused_menu();
void load_filters_data(FocusableMenu *menu);
void load_tags_data(FocusableMenu *menu);
void load_tasks_data(FocusableMenu *menu, const char *filter);
void update_task_details();
void refresh_tasks_view();
void refresh_tags_view();
void refrech_all_views();
void switch_focus_to_next();
void focus_menu_by_type(MenuType type);
void handle_filter_selection();
void handle_tag_selection();
void set_filter_view(ViewMode mode, int filter_index);
void set_tag_view(const char *tag_name, int tag_index);
void cleanup_app_state();


WINDOW *create_newwin(int height, int width, int starty, int startx);
void destroy_win(WINDOW *win);
void update_time_top_bar(WINDOW *win, int y_pos, int x_pos);
void print_menu(WINDOW *menu_win, int highlight, char **list, int count);

void add_focusable_window(WINDOW *win, MENU *menu);

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string);

void show_task_details(WINDOW *win ,Task *t);

WINDOW* create_top_bar();
WINDOW* create_task_details_window();

void update_menu_highlighting();

// form:
void handle_add_task();
WINDOW *create_form_window(void);
void destroy_form_window(WINDOW *form_win);
void show_add_task_form(sqlite3 *db);

void show_tag_menu(WINDOW *parent_win, char *selected_tag);
void show_add_tag_win(WINDOW *parent_win, char *selected_tag, sqlite3 *db);

#endif