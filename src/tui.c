#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <panel.h>
#include <form.h>
#include <locale.h> 

#include "tui.h"
#include "database.h"
#include "utils.h"

const char *FILTER_NAMES[] = {
    "All",
    "Today",
    "Tomorrow",
    "Over-Due",
    "Completed"
};

const int FILTER_COUNT = sizeof(FILTER_NAMES) / sizeof(char *);

int src_width, src_height;

AppState app_state = {0};

void init_app_state(sqlite3 *db){
    app_state.db = db;
    app_state.mode = VIEW_MODE_ALL;
    app_state.current_filter = strdup("1=1");
    app_state.active_filter_index = 0;
    app_state.active_tag_index = -1;
    app_state.selected_tag_name = NULL;
    app_state.menus = NULL;
    app_state.current_focus_idx = 0;
}

void init_tui(sqlite3 *db){

    setlocale(LC_ALL, "");

    initscr();          
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);
    refresh();

    getmaxyx(stdscr, src_height, src_width);

    init_app_state(db);

    app_state.top_bar_win = create_newwin(3, src_width, 0, 0);
    app_state.top_bar_panel = new_panel(app_state.top_bar_win);

    FocusableMenu *filters_menu = creat_focusable_menu(MENU_TYPE_FILTER, "Filters", 10, SIDE_BAR_WIDTH, 3, 0);
    load_filters_data(filters_menu);
    register_menu(filters_menu);

    FocusableMenu *tags_menu = creat_focusable_menu(MENU_TYPE_TAG, "Tags", src_height - 13, SIDE_BAR_WIDTH, 13, 0);
    load_tags_data(tags_menu);
    register_menu(tags_menu);

    int window_height = src_height - 3;
    int window_width = (src_width - SIDE_BAR_WIDTH) / 2;
    FocusableMenu *tasks_menu = creat_focusable_menu(MENU_TYPE_TASK, "Tasks", window_height, window_width, 3, SIDE_BAR_WIDTH);
    load_tasks_data(tasks_menu, "1=1");
    register_menu(tasks_menu);

    int x_pos = SIDE_BAR_WIDTH + window_width;
    app_state.details_win = newwin(window_height, window_width, 3, x_pos);
    app_state.details_panel = new_panel(app_state.details_win);

    print_in_middle(app_state.details_win, 1, 0, window_width, "Details");
    mvwaddch(app_state.details_win, 2, 0, ACS_LTEE);
    mvwhline(app_state.details_win, 2, 1, ACS_HLINE, window_width - 2);
    mvwaddch(app_state.details_win, 2, window_width - 1, ACS_RTEE);

    app_state.menus[0]->is_focused = 1;
    update_menu_highlighting();
    update_task_details();

    update_panels();
    doupdate();

    int c;
    while(1){
        update_time_top_bar(app_state.top_bar_win, 1, src_width);

        FocusableMenu *current_menu = get_focused_menu();
        c = wgetch(current_menu -> win);

        switch (c)
        {
            case 9:{
                switch_focus_to_next();
                continue;
            }

            case KEY_UP:{
                if (current_menu->menu && item_count(current_menu->menu) > 0) {
                    menu_driver(current_menu -> menu, REQ_UP_ITEM);

                    if (current_menu->type == MENU_TYPE_TASK){
                        update_task_details();
                    }
                }
                break;
            }
            case KEY_DOWN:{
                if (current_menu->menu && item_count(current_menu->menu) > 0) {
                    menu_driver(current_menu -> menu, REQ_DOWN_ITEM);

                    if (current_menu->type == MENU_TYPE_TASK){
                        update_task_details();
                    }
                }
                break;
            }
            case 10: {
                if (current_menu->type == MENU_TYPE_FILTER){
                    handle_filter_selection();
                } else if (current_menu->type == MENU_TYPE_TAG){
                    handle_tag_selection();
                }
                pos_menu_cursor(current_menu -> menu);
                break;
            }
            case 'a':
            case 'A':{
                handle_add_task();
                break;
            }
            case 'c':
            case 'C':{
                handle_task_status();
                break;
            }
            case 'q':{
                cleanup_app_state();
                endwin();
                exit(0);
            }
            default:
				break;
        }
        update_panels();
        doupdate();
    }   
    cleanup_app_state();
    endwin();
}

FocusableMenu *creat_focusable_menu(MenuType type, char *title, int height, int width, int starty, int startx){

    FocusableMenu *menu = malloc(sizeof(FocusableMenu));

    menu->type = type;
    menu->title = strdup(title);

    menu->height = height;
    menu->width = width;
    menu->starty = starty;
    menu->startx = startx;
    menu->is_focused = 0;
    menu->selected_index = 0;

    menu->data = (MenuData){0};

    menu->win = newwin(height, width, starty, startx);
    box(menu->win, 0, 0);
    menu->panel = new_panel(menu->win);
    keypad(menu->win, true);

    menu->subwin = NULL;
    menu->menu = NULL;
    menu->items = NULL;
    
    return menu;
}

void free_menu_data(FocusableMenu *menu){
    if (menu->type == MENU_TYPE_FILTER) return; // the filter menu should not be freed if not for the cleanup, it doesn't need it's afixed menu that doens't get changed in run time

    if (menu->items){
        for (int i = 0; menu->items[i] != NULL; i++){
            free_item(menu->items[i]);
        }
        free(menu->items);
        menu->items = NULL;
    }

    switch (menu->type)
    {
    case MENU_TYPE_FILTER:
        break;
    case MENU_TYPE_TAG:
        if (menu->data.tag_list){
            for (int i = 0; i < menu->data.tag_count; i++){
                free(menu->data.tag_list[i]);
            }
            free(menu->data.tag_list);
            menu->data.tag_list = NULL;
            menu->data.tag_count = 0;
        }
        break;
    
    case MENU_TYPE_TASK:
        if (menu->data.task_list){
            for (int i = 0; i < menu->data.task_count; i++){
                free_task_field(&menu->data.task_list[i]);
            }
            free(menu->data.task_list);
            menu->data.task_list = NULL;
            menu->data.task_count = 0;
        }
        break;
    }
}

void register_menu(FocusableMenu *menu){
    app_state.menus = realloc(app_state.menus, (app_state.menu_count + 1) * sizeof(FocusableMenu *));
    app_state.menus[app_state.menu_count] = menu;
    app_state.menu_count++;
}

FocusableMenu *get_menu_by_type(MenuType type){
    for (int i = 0; i < app_state.menu_count; i++){
        if (app_state.menus[i]->type == type){
            return app_state.menus[i];
        }
    }
    return NULL;
}

FocusableMenu *get_focused_menu(){
    if (app_state.current_focus_idx >= 0 && app_state.current_focus_idx < app_state.menu_count){
        return app_state.menus[app_state.current_focus_idx];
    }
    return NULL;
}

void load_filters_data(FocusableMenu *menu){
    if(menu->menu == NULL){
        menu->data.filter_count = FILTER_COUNT;
        menu->data.filter_list = (char **)FILTER_NAMES;

        menu->items = malloc((menu->data.filter_count + 1) * sizeof(ITEM *));
        for (int i = 0; i < menu->data.filter_count; i++){
            menu->items[i] = new_item(menu->data.filter_list[i], NULL);
        }
        menu->items[menu->data.filter_count] = NULL;

        menu->menu = new_menu(menu->items);
        set_menu_win(menu->menu, menu->win);
        menu->subwin = derwin(menu->win, 6, menu->width - 4, 4, 2);
        set_menu_sub(menu->menu, menu->subwin);
        set_menu_mark(menu->menu, " * ");

        box(menu->win, 0, 0);
        print_in_middle(menu->win, 1, 0, menu->width, menu->title);
        mvwaddch(menu->win, 2, 0, ACS_LTEE);
        mvwhline(menu->win, 2, 1, ACS_HLINE, menu->width - 2);
        mvwaddch(menu->win, 2, menu->width - 1, ACS_RTEE);
    }
    post_menu(menu->menu);
    wrefresh(menu->win);
}

void load_tags_data(FocusableMenu *menu){
    if (menu->menu){
        unpost_menu(menu->menu);
        set_menu_items(menu->menu, NULL);
    }

    werase(menu->win);

    free_menu_data(menu);

    load_tags(app_state.db, &menu->data.tag_list, &menu->data.tag_count);

    if (menu->data.tag_count > 0){
        menu->items = malloc((menu->data.tag_count + 1) * sizeof(ITEM *));
        for (int i = 0; i < menu->data.tag_count; i++){
            menu->items[i] = new_item(menu->data.tag_list[i], NULL);
        }
        menu->items[menu->data.tag_count] = NULL;

        if (!menu->menu){
            menu->menu = new_menu(menu->items);
            set_menu_win(menu->menu, menu->win);
            menu->subwin = derwin(menu->win , menu->height - 4, menu->width - 4, 4, 2);
            set_menu_sub(menu->menu, menu->subwin);
        } else {
            set_menu_items(menu->menu, menu->items);
            set_menu_win(menu->menu, menu->win);
            set_menu_sub(menu->menu, menu->subwin);
        }

        set_menu_format(menu->menu, 35, 1);
        set_menu_mark(menu->menu, " * ");

        post_menu(menu->menu);
        
        if (menu->selected_index < menu->data.tag_count){
            set_current_item(menu->menu, menu->items[menu->selected_index]);
        }
    }
    box(menu->win, 0, 0);
    print_in_middle(menu->win, 1, 0, menu->width, menu->title);
    mvwaddch(menu->win, 2, 0, ACS_LTEE);
    mvwhline(menu->win, 2, 1, ACS_HLINE, menu->width - 2);
    mvwaddch(menu->win, 2, menu->width - 1, ACS_RTEE);

    wrefresh(menu->win);
}

void load_tasks_data(FocusableMenu *menu, const char *filter){
    if (menu->menu){
        unpost_menu(menu->menu);
        set_menu_items(menu->menu, NULL);
    }

    werase(menu->win);

    free_menu_data(menu);

    load_tasks_fillterd(app_state.db, &menu->data, filter);

    if (menu->data.task_count > 0){
        menu->items = malloc((menu->data.task_count + 1) * sizeof(ITEM *));

        for (int i = 0; i < menu->data.task_count; i++){
            menu->items[i] = new_item(menu->data.task_list[i].title, NULL);
            set_item_userptr(menu->items[i], &menu->data.task_list[i]);
        }
        menu->items[menu->data.task_count] = NULL;

        if (!menu->menu){
            menu->menu = new_menu(menu->items);
            set_menu_win(menu->menu, menu->win);
            menu->subwin = derwin(menu->win, menu->height - 6, menu->width - 4, 4, 2);
            set_menu_sub(menu->menu, menu->subwin);
        } else {
            set_menu_items(menu->menu, menu->items);
            set_menu_win(menu->menu, menu->win);
            set_menu_sub(menu->menu, menu->subwin);
        }

        set_menu_format(menu->menu, 45, 1);
        set_menu_mark(menu->menu, " * ");

        post_menu(menu->menu);

        if (menu->selected_index < menu->data.task_count){
            set_current_item(menu->menu, menu->items[menu->selected_index]);
        }
    }

    box(menu->win, 0, 0);
    print_in_middle(menu->win, 1, 0, menu->width, menu->title);
    mvwaddch(menu->win, 2, 0, ACS_LTEE);
    mvwhline(menu->win, 2, 1, ACS_HLINE, menu->width - 2);
    mvwaddch(menu->win, 2, menu->width - 1, ACS_RTEE);
    
    wrefresh(menu->win);
}

void update_task_details(){
    FocusableMenu *tasks_menu = get_menu_by_type(MENU_TYPE_TASK);

    if(tasks_menu && tasks_menu->menu && item_count(tasks_menu->menu) > 0){
        ITEM *current = current_item(tasks_menu->menu);
        if (current){
            Task *task = (Task *)item_userptr(current);
            show_task_details(app_state.details_win, task);
        }
    } else {
        werase(app_state.details_win);
        box(app_state.details_win, 0, 0);
        int window_width = (src_width - SIDE_BAR_WIDTH) / 2;
        print_in_middle(app_state.details_win, 1, 0, window_width, "Details");
        mvwaddch(app_state.details_win, 2, 0, ACS_LTEE);
        mvwhline(app_state.details_win, 2, 1, ACS_HLINE, window_width - 2);
        mvwaddch(app_state.details_win, 2, window_width - 1, ACS_RTEE);
        mvwprintw(app_state.details_win, 4, 2, "No task selected");
        wrefresh(app_state.details_win);
    }
}

void refresh_tasks_view(){
    FocusableMenu *tasks_menu = get_menu_by_type(MENU_TYPE_TASK);
    if(!tasks_menu) return;

    if(tasks_menu->menu && item_count(tasks_menu->menu) > 0){
        tasks_menu->selected_index = item_index(current_item(tasks_menu->menu));
    }

    load_tasks_data(tasks_menu, app_state.current_filter);
    update_task_details();
}

void refresh_tags_view(){
    FocusableMenu *tags_menu = get_menu_by_type(MENU_TYPE_TAG);
    if(!tags_menu) return;
    load_tags_data(tags_menu);
}

void refrech_all_views(){
    for (int i = 0; i < app_state.menu_count; i++) {
        FocusableMenu *menu = app_state.menus[i];
        
        if (menu->type == MENU_TYPE_TASK) {
            load_tasks_data(menu, app_state.current_filter);
        } else if (menu->type == MENU_TYPE_TAG) {
            load_tags_data(menu);
        }
    }
    
    update_task_details();
    update_menu_highlighting();
    
    update_panels();
    doupdate();
}

void switch_focus_to_next(){
    FocusableMenu *current = get_focused_menu();
    if(current){
        current->is_focused = 0;
    }

    app_state.current_focus_idx = (app_state.current_focus_idx + 1) % app_state.menu_count;

    FocusableMenu *next = get_focused_menu();
    if(next){
        next->is_focused = 1;
    }

    update_menu_highlighting();

    if (next && next->type == MENU_TYPE_TASK){
        update_task_details();
    }
}

void focus_menu_by_type(MenuType type){
    for (int i = 0; i < app_state.menu_count; i++){
        app_state.menus[i]->is_focused = 0;

        if (app_state.menus[i]->type == type){
            app_state.current_focus_idx = i;
            app_state.menus[i]->is_focused = 1;
        }
    }
    update_menu_highlighting();

    if (type == MENU_TYPE_TASK) {
        update_task_details();
    }

    update_panels();
    doupdate();
}

void handle_filter_selection(){
    FocusableMenu *filter_menu = get_focused_menu();
    if (!filter_menu || filter_menu->type != MENU_TYPE_FILTER) return;

    ITEM *current = current_item(filter_menu->menu);
    if (!current) return;

    const char *sel = item_name(current);
    int idx = item_index(current);

    ViewMode mode;
    if (strcmp(sel, "All") == 0) mode = VIEW_MODE_ALL;
    else if (strcmp(sel, "Today") == 0) mode = VIEW_MODE_TODAY;
    else if (strcmp(sel, "Tomorrow") == 0) mode = VIEW_MODE_TOMORROW;
    else if (strcmp(sel, "Over-Due") == 0) mode = VIEW_MODE_OVERDUE;
    else mode = VIEW_MODE_COMPLETED;

    set_filter_view(mode, idx);
    refresh_tasks_view();
    focus_menu_by_type(MENU_TYPE_TASK);

    update_panels();
    doupdate();
}

void handle_tag_selection(){
    FocusableMenu *tags_menu = get_focused_menu();
    if (!tags_menu || tags_menu->type != MENU_TYPE_TAG) return;

    ITEM *current = current_item(tags_menu->menu);
    if (!current) return;

    const char *tag = item_name(current);
    int idx = item_index(current);

    set_tag_view(tag, idx);
    refresh_tasks_view();
    focus_menu_by_type(MENU_TYPE_TASK);

    update_panels();
    doupdate();
}

void set_filter_view(ViewMode mode, int filter_index){
    app_state.mode = mode;
    app_state.active_filter_index = filter_index;
    app_state.active_tag_index = -1;

    if (app_state.selected_tag_name){
        free(app_state.selected_tag_name);
        app_state.selected_tag_name = NULL;
    }

    if (app_state.current_filter){
        free(app_state.current_filter);
    }
    // TODO: fix the incorect time where clause... why not now? man are you really asking me this question now ?? i'm working or refactoring the shitt house that i built, i can't think of anything right now and i don't want to until i complete the refatoring the code returns to a working state 
    switch(mode){
        case VIEW_MODE_ALL:
            app_state.current_filter = strdup("1=1");
            break;
        case VIEW_MODE_TODAY:
            app_state.current_filter = strdup("date(due_date, 'unixepoch')=date('now')");
            break;
        case VIEW_MODE_TOMORROW:
            app_state.current_filter = strdup("date(due_date, 'unixepoch')=date('now','+1 day')");
            break;
        case VIEW_MODE_OVERDUE:
            app_state.current_filter = strdup("date(due_date, 'unixepoch')<date('now')");
            break;
        case VIEW_MODE_COMPLETED:
            app_state.current_filter = strdup("t.status=1");
            break;
        default:
            app_state.current_filter = strdup("1=1");
        }

        FocusableMenu *filter_menu = get_menu_by_type(MENU_TYPE_FILTER);
        if (filter_menu){
            filter_menu->selected_index = filter_index;
        }

        FocusableMenu *tags_menu = get_menu_by_type(MENU_TYPE_TAG);
        if (tags_menu){
            tags_menu->selected_index = -1;
        }
}

void set_tag_view(const char *tag_name, int tag_index){
    app_state.mode = VIEW_MODE_BY_TAG;
    app_state.active_tag_index = tag_index;
    app_state.active_filter_index = -1;

    if (app_state.current_filter) {
        free(app_state.current_filter);
    }
    if (app_state.selected_tag_name) {
        free(app_state.selected_tag_name);
    }

    app_state.selected_tag_name = strdup(tag_name);

    app_state.current_filter = malloc(31);
    snprintf(app_state.current_filter, 31, "tg.name='%s'", tag_name);

    FocusableMenu *tags_menu = get_menu_by_type(MENU_TYPE_TAG);
    if (tags_menu){
        tags_menu->selected_index = tag_index;
    }

    FocusableMenu *filter_menu = get_menu_by_type(MENU_TYPE_FILTER);
    if (filter_menu){
        filter_menu->selected_index = -1;
    }
}

void cleanup_app_state(){
    if (app_state.current_filter) free(app_state.current_filter);
    if (app_state.selected_tag_name) free(app_state.selected_tag_name);

    for (int i = 0; i < app_state.menu_count; i++){
        FocusableMenu *menu = app_state.menus[i];

        if (menu->menu){
            unpost_menu(menu->menu);
            free_menu(menu->menu);
        }

        free_menu_data(menu);

        if (menu->panel) del_panel(menu->panel);
        if (menu->subwin) delwin(menu->subwin);
        if (menu->win) delwin(menu->win);

        free(menu->title);
        free(menu);
    }
    free(app_state.menus);

    if (app_state.details_panel) del_panel(app_state.details_panel);
    if (app_state.details_win) delwin(app_state.details_win);
    if (app_state.top_bar_panel) del_panel(app_state.top_bar_panel);
    if (app_state.top_bar_win) delwin(app_state.top_bar_win);
}

WINDOW *create_newwin(int src_height, int src_width, int starty, int startx){
    WINDOW *local_win;

    local_win = newwin(src_height, src_width, starty, startx);
    box(local_win, 0, 0);

    wrefresh(local_win);

    return local_win;
}

void update_time_top_bar(WINDOW *win, int y_pos, int src_width){
    time_t now = time(NULL);
    char time_str[64];
    clrtoeol();
    strftime(time_str,sizeof(time_str),"%a %d %b %H:%M",localtime(&now));
    wattron(win,A_ITALIC | A_BOLD);
    wmove(win,y_pos,(src_width - strlen(time_str))/2);
    wprintw(win,"%s",time_str);
    wattroff(win,A_ITALIC | A_BOLD);
    box(win, 0,0);
    wrefresh(win);
}

void print_in_middle(WINDOW *win, int starty, int startx, int src_width, char *string){

    int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(src_width == 0)
		src_width = 80;

	length = strlen(string);
	temp = (src_width - length)/ 2;
	x = startx + (int)temp;
	mvwprintw(win, y, x, "%s", string);
	refresh();
}

void show_task_details(WINDOW *win, Task *t) {
    if (!t) {
        werase(win);
        box(win, 0,0);
        print_in_middle(win, 1, 0, (src_width-SIDE_BAR_WIDTH)/2, "Details");
        mvwaddch(win, 2, 0, ACS_LTEE);
        mvwhline(win, 2, 1, ACS_HLINE, ((src_width-SIDE_BAR_WIDTH)/2)-2);
        mvwaddch(win, 2, ((src_width-SIDE_BAR_WIDTH)/2)-1, ACS_RTEE);
        mvwprintw(win, 4, 2, "No task selected");
        wrefresh(win);
        return;
    }

    werase(win);
    box(win, 0,0);

    mvwaddch(win, 2, 0, ACS_LTEE);
    mvwhline(win, 2, 1, ACS_HLINE, ((src_width-SIDE_BAR_WIDTH)/2)-2);
    mvwaddch(win, 2, ((src_width-SIDE_BAR_WIDTH)/2)-1, ACS_RTEE);

    mvwprintw(win, 3, 2, "ID:   %d", t->id);
    mvwprintw(win, 4, 2, "Title: %s", t->title);
    print_in_middle(win, 1,0,(src_width-SIDE_BAR_WIDTH)/2, t->title);
    mvwprintw(win, 5, 2, "Desc:  %s", t->desc);
    mvwprintw(win, 6, 2, "Status: %s", t->status ? "Completed" : "Pending");

    time_t created_at = (time_t)t->created_at;
    struct tm *lt1 = localtime(&created_at);

    char created_at_buf[64];
    strftime(created_at_buf, sizeof(created_at_buf), "%d-%m-%Y %H:%M:%S", lt1);

    mvwprintw(win, 7, 2, "Created at: %s", created_at_buf);

    time_t due_date = (time_t)t->due_date;
    struct tm *lt2 = localtime(&due_date);

    char due_date_buf[64];
    strftime(due_date_buf, sizeof(due_date_buf), "%d-%m-%Y %H:%M:%S", lt2);

    mvwprintw(win, 8, 2, "Due at: %s", due_date_buf);

    wrefresh(win);
}

void update_menu_highlighting(void){
    for (int i = 0; i < app_state.menu_count; i++){
        FocusableMenu *menu = app_state.menus[i];
        
        if(menu->is_focused){
            // Focused menu
            set_menu_fore(menu->menu, A_REVERSE | A_BOLD);
            set_menu_back(menu->menu, A_NORMAL);
            set_menu_mark(menu->menu, " * ");
            
            wattron(menu->win, A_BOLD);
            box(menu->win, 0, 0);
            wattroff(menu->win, A_BOLD);
        } else {
            // Non-focused menu
            set_menu_fore(menu->menu, A_DIM);
            set_menu_back(menu->menu, A_DIM);
            
            // Show selection indicator for filters/tags even when not focused
            if(menu->type == MENU_TYPE_FILTER && app_state.active_filter_index >= 0) {
                // Filter menu show which filter is active
                set_menu_mark(menu->menu, " > ");
                if (app_state.active_filter_index < item_count(menu->menu)) {
                    set_current_item(menu->menu, menu->items[app_state.active_filter_index]);
                }
            } else if(menu->type == MENU_TYPE_TAG && app_state.active_tag_index >= 0) {
                // Tag menu
                set_menu_mark(menu->menu, " > ");
                if (app_state.active_tag_index < item_count(menu->menu)) {
                    set_current_item(menu->menu, menu->items[app_state.active_tag_index]);
                }
            } else {
                set_menu_mark(menu->menu, "   ");
            }
            box(menu->win, 0, 0);
        }
        wrefresh(menu->win);
    }
    refresh();
}

// form:

void handle_add_task(){
    show_add_task_form(app_state.db);

    FocusableMenu *current = get_focused_menu();
    if (current->type != MENU_TYPE_TASK){
        focus_menu_by_type(MENU_TYPE_TASK);
    }

    refrech_all_views();
}

WINDOW *create_form_window(void){
    int form_height = 24;
    int form_width = 70;
    int start_y = (src_height - form_height)/2;
    int start_x = (src_width - form_width)/2;

    WINDOW *form_win = newwin(form_height, form_width, start_y, start_x);
    box(form_win, 0, 0);

    wattron(form_win, A_BOLD);
    print_in_middle(form_win, 1, 0, form_width, "Add New Task");
    wattroff(form_win, A_BOLD);

    mvwaddch(form_win, 2, 0, ACS_LTEE);
    mvwhline(form_win, 2, 1, ACS_HLINE, form_width - 2);
    mvwaddch(form_win, 2, form_width - 1, ACS_RTEE);


    return form_win;
}

void destroy_form_window(WINDOW *form_win) {
    werase(form_win);
    wrefresh(form_win);
    delwin(form_win);
    touchwin(stdscr);
    refresh();
}

void show_add_task_form(sqlite3 *db) {
    FIELD *fields[5];
    FORM *form;
    WINDOW *form_win, *form_subwin;
    PANEL *form_panel;

    form_win = create_form_window();
    keypad(form_win, TRUE);

    char selected_tag[31] = "None"; // the deffault tag is none (no tag) 

    mvwprintw(form_win, 4, 2, "Title:");
    mvwprintw(form_win, 6, 2, "Description: ");
    mvwprintw(form_win, 12, 2, "Due Date:");
    
    mvwprintw(form_win, 15, 2, "Tag:");
    mvwprintw(form_win, 15, 16, "[%s] Press TAB to select", selected_tag);
    
    mvwprintw(form_win, 22, 2, "ENTER: Save | ESC: Cancel | ↑↓: Navigate | TAB on Tag: Select");

    form_subwin = derwin(form_win, 14, 52, 4, 16);

    fields[0] = new_field(1, 50, 0, 0, 0, 0); //Title
    fields[1] = new_field(4, 50, 2, 0, 0, 0); // Descreption
    fields[2] = new_field(1, 16, 8, 0, 0, 0); // Due date
    fields[3] = new_field(1, 30, 11, 0, 0, 0); // Tag
    fields[4] = NULL;

    for(int i = 0; i < 4; i++) {
        set_field_back(fields[i], A_UNDERLINE);
        field_opts_off(fields[i], O_AUTOSKIP);
    }

    field_opts_off(fields[3], O_EDIT);
    // field_opts_off(field[3], O_ACTIVE);
    set_field_buffer(fields[3], 0, selected_tag);

    form = new_form(fields);
    set_form_win(form, form_win);
    set_form_sub(form, form_subwin);
    post_form(form);

    form_panel = new_panel(form_win);
    top_panel(form_panel);
    
    update_panels();
    doupdate();

    wattron(form_win, A_DIM);
    mvwprintw(form_win, 12, 35, "HH:MM DD/MM/YYYY");
    wattroff(form_win, A_DIM);
    wrefresh(form_win);

    curs_set(1);
    pos_form_cursor(form);
    
    int ch;
    int current_field = 0;

    while ((ch = wgetch(form_win)) != 27) {
        mvwprintw(form_win, 17, 16, "%-45s", "");
        wrefresh(form_win);

        switch (ch) {
            case KEY_DOWN:
                if (current_field < 3){
                    form_driver(form, REQ_NEXT_FIELD);
                    form_driver(form, REQ_END_LINE);
                    current_field++;
                }
                break;
            case KEY_UP:
                if (current_field > 0){
                    form_driver(form, REQ_PREV_FIELD);
                    form_driver(form, REQ_END_LINE);
                    current_field--;
                }
                break;
            case 9:  // TAB key
                if (current_field == 3) {  // On tag field
                    show_tag_menu(form_win, selected_tag);
                    if (strcmp(selected_tag, "-- Add new tag --") == 0){
                        show_add_tag_win(form_win, selected_tag, db);
                    }

                    set_field_buffer(fields[3], 0, selected_tag);
                    mvwprintw(form_win, 15, 16, "[%-15s] Press TAB to select", selected_tag);
                    wrefresh(form_win);
                } else {

                    if (current_field < 3) {
                        form_driver(form, REQ_NEXT_FIELD);
                        form_driver(form, REQ_END_LINE);
                        current_field++;
                    }
                }
                break;
            case 10:
                if (current_field < 3) {
                    form_driver(form, REQ_NEXT_FIELD);
                    form_driver(form, REQ_END_LINE);
                    current_field++;
                } else {
                    form_driver(form, REQ_VALIDATION);

                    char *title = trim_fieldbuf(field_buffer(fields[0], 0));

                    if (!title || title[0] == '\0'){
                        mvwprintw(form_win, 17, 16, "Title cannot be empty. Please enter a title.");
                        current_field = 0;
                        set_current_field(form, fields[0]);
                        pos_form_cursor(form);
                        wrefresh(form_win);
                        free(title);
                        continue;
                    }

                    char *date = trim_fieldbuf(field_buffer(fields[2], 0));

                    if (date && date[0] != '\0'){ // the due date is optional
                        if(!validate_datetime(date)){
                            mvwprintw(form_win, 17, 16, "Invalid date. Use HH:MM DD/MM/YYYY.");
                            current_field = 2;
                            set_current_field(form, fields[2]);
                            pos_form_cursor(form);
                            wrefresh(form_win);
                            free(date);
                            continue;
                        }
                    }
                    
                    TaskFormData new_task = {0};

                    strcpy(new_task.title, title);

                    char *desc = trim_fieldbuf(field_buffer(fields[1], 0));
                    if (desc && desc[0] != '\0'){
                        strcpy(new_task.description, desc);
                    }else{
                        strcpy(new_task.description, "NULL");
                    }
                    

                    // Convert the string date to a unix timestamp
                    if (date && date[0] != '\0'){ 
                        struct tm tm_info = {.tm_sec=0};
                        char * res = strptime(date, "%H:%M %d/%m/%Y", &tm_info);
                        if (res != NULL){
                            new_task.due_date = mktime(&tm_info);
                        } else {
                            new_task.due_date = -1;
                        }
                    } else {
                        new_task.due_date = -1;
                    }

                    strcpy(new_task.tag_name, trim_fieldbuf(field_buffer(fields[3], 0)));

                    int rc = insert_new_task(db, new_task);

                    if (!rc){
                        mvwprintw(form_win, 18, 16, "Task saved!");
                        wrefresh(form_win);
                        napms(1000);
                        goto exit;
                    } else {
                        mvwprintw(form_win, 18, 16, "Error: Failed to save task.");
                        wrefresh(form_win); 
                    }
                }
                break;
            case KEY_BACKSPACE:
            case 127:
            case 8:
                form_driver(form, REQ_DEL_PREV);
                break;
                
            case KEY_DC:  // Delete key
                form_driver(form, REQ_DEL_CHAR);
                break;
                
            case KEY_LEFT:
                form_driver(form, REQ_PREV_CHAR);
                break;
                
            case KEY_RIGHT:
                form_driver(form, REQ_NEXT_CHAR);
                break;

            default:
                form_driver(form, ch);
                break;
        }
        wrefresh(form_win);
    }

    exit:
    
    curs_set(0);

    // Cleanup
    unpost_form(form);
    free_form(form);
    for(int i = 0; i < 3; i++) {
        free_field(fields[i]);
    }

    hide_panel(form_panel);
    del_panel(form_panel);
    delwin(form_subwin);
    destroy_form_window(form_win);
}

void show_tag_menu(WINDOW *parent_win,char *selected_tag) {
    FocusableMenu *tags_menu = get_menu_by_type(MENU_TYPE_TAG);
    if (!tags_menu) return;

    char **tags = tags_menu->data.tag_list;
    int tag_count = tags_menu->data.tag_count;
    
    int menu_height = 25 + 5;
    int menu_width = 30;
    int start_y = (getmaxy(parent_win) - menu_height) / 2;
    int start_x = (getmaxx(parent_win) - menu_width) / 2;

    curs_set(0);
    
    WINDOW *menu_win = newwin(menu_height, menu_width, getbegy(parent_win) + start_y, getbegx(parent_win) + start_x);
    
    box(menu_win, 0, 0);
    mvwprintw(menu_win, 1, 2, "Select Tag (↑↓ ENTER)");
    mvwaddch(menu_win, 2, 0, ACS_LTEE);
    mvwhline(menu_win, 2, 1, ACS_HLINE, menu_width - 2);
    mvwaddch(menu_win, 2, menu_width - 1, ACS_RTEE);

    WINDOW *menu_subwin = derwin(menu_win, 26, 26, 3, 2);
    
    ITEM **menu_items = (ITEM **)calloc(tag_count+3, sizeof(ITEM *));

    menu_items[0] = new_item("None", NULL);
    menu_items[1] = new_item("-- Add new tag --", NULL);
    for (int i = 0; i < tag_count; i++) {
        menu_items[i+2] = new_item(tags[i], NULL);
    }
    menu_items[tag_count+2] = NULL;

    MENU *the_menu =  new_menu(menu_items);
    
    set_menu_win(the_menu, menu_win);
    set_menu_sub(the_menu, menu_subwin);
    set_menu_mark(the_menu, " > ");
    set_menu_format(the_menu, 26, 1);

    post_menu(the_menu);

    PANEL *menu_panel = new_panel(menu_win);

    int ch;
    
    keypad(menu_win, TRUE);

    update_panels();
    doupdate();
    
    while (1) {
        ch = wgetch(menu_win);
        
        switch (ch) {
            case KEY_UP:
                menu_driver(the_menu, REQ_PREV_ITEM);
                break;
            case KEY_DOWN:
                menu_driver(the_menu, REQ_NEXT_ITEM);
                break;
            case 10:
                strcpy(selected_tag, item_name(current_item(the_menu)));
                goto exit_menu;
                break;
            case 27:
                if (strcmp(selected_tag, "-- Add new tag --") == 0){
                    strcpy(selected_tag, "None");
                }
                goto exit_menu;
                break;
        }
    }
    
exit_menu:
    curs_set(1);
    hide_panel(menu_panel);
    unpost_menu(the_menu);
    free_menu(the_menu);

    for (int i = 0; i < tag_count+3; i++){
        free_item(menu_items[i]);
    }
    free(menu_items);

    del_panel(menu_panel);
    delwin(menu_subwin);
    delwin(menu_win);
    update_panels();
    doupdate();

    return ;
}

void show_add_tag_win(WINDOW *parent_win, char *selected_tag, sqlite3 *db){

    int win_height = 12;
    int win_width = 35;
    int start_y = (getmaxy(parent_win) - win_height) / 2;
    int start_x = (getmaxx(parent_win) - win_width) / 2;
    WINDOW *add_tag_win = newwin(win_height, win_width, getbegy(parent_win) + start_y, getbegx(parent_win) + start_x);

    keypad(add_tag_win, TRUE);

    box(add_tag_win, 0, 0);
    mvwprintw(add_tag_win, 1, 12, "Add New Tag");
    mvwaddch(add_tag_win, 2, 0, ACS_LTEE);
    mvwhline(add_tag_win, 2, 1, ACS_HLINE, win_width - 2);
    mvwaddch(add_tag_win, 2, win_width - 1, ACS_RTEE);

    mvwprintw(add_tag_win, 4, 1, "Tag name: ");
    mvwprintw(add_tag_win, 10, 1, "ENTER: Save | ESC: Cancel");

    PANEL *add_tag_panel = new_panel(add_tag_win);

    update_panels();
    doupdate();

    char input[31] = "";
    int pos = 0;
    int ch;

    
    curs_set(1);

    while(1){
        
        mvwprintw(add_tag_win, 6, 2,"%-30s", "");
        mvwprintw(add_tag_win, 6, 2,"%s", input);
        wmove(add_tag_win, 6, 2 + pos);
        wrefresh(add_tag_win);

        ch = wgetch(add_tag_win);

        switch (ch)
        {
        case 10:
            if (pos > 0){
                strcpy(selected_tag, input);
                int exists = is_tag_exist(db, selected_tag);
                if (exists == -1){
                    mvwprintw(add_tag_win, 8, 1, "Error checking tag existence!");
                }else if (exists == 0){
                    hide_panel(add_tag_panel);
                    del_panel(add_tag_panel);
                    delwin(add_tag_win);
                
                    return ;
                }else{
                    mvwprintw(add_tag_win, 8, 1, "The tag \"%s\" already exist...", selected_tag);
                }
            }
            break;
        case 27:
            strcpy(selected_tag, "None");
            hide_panel(add_tag_panel);
            del_panel(add_tag_panel);
            delwin(add_tag_win);

            return;
                
            break;
        case KEY_BACKSPACE:
        case 127:
        case 8:
            if (pos > 0){
                input[pos] = '\0';
                pos--;
                input[pos] = '\0';
            }
            break;
        default:
            if (ch >= 32 && ch <= 126 && pos < 30){
                input[pos] = ch;
                pos++;
                input[pos] = '\0';
            }
            break;
        }
    }

    
    hide_panel(add_tag_panel);
    del_panel(add_tag_panel);
    delwin(add_tag_win);

    return;
}

// actions 

void handle_task_status(){

    FocusableMenu *tasks_menu = get_focused_menu();

    if (!tasks_menu && tasks_menu->type != MENU_TYPE_TASK){
        fprintf(stderr, "DEBUG: this is not the task win!\n");
        return;
    }

    if(!tasks_menu->menu && item_count(tasks_menu->menu) < 0){
        return;
    }

    ITEM *current_selected_item = current_item(tasks_menu->menu);

    if (!current_selected_item){
            return;
    }

    Task *task = (Task *)item_userptr(current_selected_item);

    if(!task){
        return;
    }


    const char *action = task->status ? "Pending" : "Done";

    int height = 10;
    int width = 50;
    int start_y = (src_height - height) / 2;
    int start_x = (src_width - width) / 2;

    
    WINDOW *confirmation_win = newwin(height, width, start_y, start_x);

    box(confirmation_win, 0, 0);

    const char *title = " Confirm Status Change ";

    mvwprintw(confirmation_win, 0, (width - strlen(title)) / 2, "%s", title);

    mvwprintw(confirmation_win, 2, 2, "Task: ");
    wprintw(confirmation_win, "%.35s...", task->title); 
    mvwprintw(confirmation_win, 4, 2, "Mark this task as: ");
    wattron(confirmation_win, A_BOLD);
    wprintw(confirmation_win, "%s", action);
    wattroff(confirmation_win, A_BOLD);

    const char *help = "[ENTER] Confirm   [ESC] Cancel";
    mvwprintw(confirmation_win, height - 2, (width - strlen(help)) / 2, "%s", help);

    PANEL *conf_panel = new_panel(confirmation_win);
    top_panel(conf_panel);
    update_panels();
    doupdate();

    int ch;
    
    keypad(confirmation_win, TRUE);

    while (1){

        ch = wgetch(confirmation_win);

        switch (ch)
        {
        case 10:
            task->status = !task->status;
            // TODO: call to the function the update the status in the db side
            refresh_tasks_view();
            goto exit;
        case 27:
            goto exit;
        }
    }

exit:
    hide_panel(conf_panel);
    del_panel(conf_panel);
    delwin(confirmation_win);
    update_panels();
    doupdate();
    return;
}