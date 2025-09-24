#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>

#include "tui.h"
#include "database.h"

char *tasks_filters_list[] = {
    "All",
    "Today",
    "Tomorrow",
    "Over-Due",
    "Completed"
};

int tasks_filters_count = 5;



typedef struct _FocusableMenu {
    WINDOW *win;
    MENU *menu;
    int is_focused;
} FocusableMenu;

FocusableMenu *focusable_menus = NULL;
int num_focusable_menus = 0;
int current_focus_idx = 0;

int src_width, src_height;

void init_tui(sqlite3 *db){

    ITEM  **filter_items = NULL;
    MENU *filter_menu = NULL;
    WINDOW *filters_bar_win = NULL;
    
    ITEM **tags_items;
    MENU *tags_menu;
    WINDOW *tags_bar_win;
    int tags_count;
    char **tags_list = NULL;

    TasksPane tasks_pane = {
        .win    = NULL,
        .menu   = NULL,
        .items  = NULL,
        .tasks_struct = {.task_count = 0, .task_list = NULL}
    };


    WINDOW *task_details_win = NULL;

    initscr();          
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    refresh();

    int ch ,c;

    curs_set(0);
    getmaxyx(stdscr, src_height, src_width); 


    WINDOW *top_bar = create_top_bar();
    create_filter_menu(&filter_items, &filter_menu, &filters_bar_win);
    create_tags_menu(db, &tags_items, &tags_menu, &tags_bar_win, &tags_list, &tags_count);
    create_tasks_menu(db, &tasks_pane);
    task_details_win = create_task_details_window();

    add_focusable_window(filters_bar_win, filter_menu);
    add_focusable_window(tags_bar_win, tags_menu);
    add_focusable_window(tasks_pane.win, tasks_pane.menu);

    focusable_menus[0].is_focused = 1;
    update_window_focus();


    while((ch = getch())!='q'){
        update_time_top_bar(top_bar, 1, src_width);

        if (ch == 9){
            switch_focus();
            continue;
        }

        FocusableMenu *current_menu = &focusable_menus[current_focus_idx];
        c = wgetch(current_menu -> win);

        switch (c)
        {
            case 9:
                switch_focus();
                continue;

            case KEY_UP:
                menu_driver(current_menu -> menu, REQ_UP_ITEM);

                if (current_focus_idx == 2){
                    show_task_details(task_details_win, (Task *)item_userptr(current_item(current_menu -> menu)));
                }

                break;

            case KEY_DOWN:
                menu_driver(current_menu -> menu, REQ_DOWN_ITEM);

                if (current_focus_idx == 2){
                    show_task_details(task_details_win, (Task *)item_userptr(current_item(current_menu -> menu)));
                }

                break;

            case 10: {
                    const char *sel = item_name(current_item(current_menu->menu));

                    if (current_focus_idx == 0){
                        const char *where;
                        if      (strcmp(sel, "All")      == 0) where = "1=1";
                        else if (strcmp(sel, "Today")    == 0) where = "date(due_date, 'unixepoch')=date('now')";
                        else if (strcmp(sel, "Tomorrow") == 0) where = "date(due_date, 'unixepoch')=date('now','+1 day')";
                        else if (strcmp(sel, "Over-Due") == 0) where = "date(due_date, 'unixepoch')<date('now')";
                        else where = "t.status=1";

                        reload_tasks_menu(db, &tasks_pane, where);

                        current_focus_idx = 2;
                        for (int i = 0; i < num_focusable_menus; i++){ 
                            focusable_menus[i].is_focused = 0;
                        }
                        focusable_menus[2].is_focused = 1;
                        update_window_focus();

                    } else if (current_focus_idx == 1){
                        char where[256];
                        snprintf(where, sizeof(where),"tg.name='%s'", sel);
                        reload_tasks_menu(db, &tasks_pane, where);
                        
                        current_focus_idx = 2;
                        for (int i = 0; i < num_focusable_menus; i++){ 
                            focusable_menus[i].is_focused = 0;
                        }
                        focusable_menus[2].is_focused = 1;
                        update_window_focus();

                    }
                
                pos_menu_cursor(current_menu -> menu);
                break;
            }
            case 'q':
                cleanup_menus();
                destroy_win(top_bar);
                destroy_win(filters_bar_win);   
                destroy_win(tags_bar_win);
                endwin();
                exit(0);
            default:
				mvprintw(src_height-2, 1, "Charcter pressed is = %3d\nHopefully it can be printed as '%c'", c, c);
				refresh();
				break;
        }
        clrtoeol();
        refresh();
    }   
    refresh();

    cleanup_menus();
    destroy_win(top_bar);
    destroy_win(filters_bar_win);   
    destroy_win(tags_bar_win);
    endwin();
}

WINDOW *create_newwin(int src_height, int src_width, int starty, int startx){
    WINDOW *local_win;

    local_win = newwin(src_height, src_width, starty, startx);
    box(local_win, 0, 0);

    wrefresh(local_win);

    return local_win;
}

void destroy_win(WINDOW *win){
    wborder(win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    clrtoeol();
    wrefresh(win);
    delwin(win);
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

void print_menu(WINDOW *menu_win, int highlight, char **list, int count){
    int x, y, i;
    x = 2;
    y = 2;

    for (i = 0; i < count; i++){
        if(highlight == i){
            wattron(menu_win, A_REVERSE);
            mvwprintw(menu_win, y, x, "%s", list[i]);
            wattroff(menu_win, A_REVERSE);
        }else{
            mvwprintw(menu_win, y, x, "%s", list[i]);
        }
        y++;
    }
    wrefresh(menu_win);
}

void add_focusable_window(WINDOW *win, MENU *menu){
    num_focusable_menus++;
    focusable_menus = (FocusableMenu *)realloc(focusable_menus, num_focusable_menus * sizeof(FocusableMenu));
    if (focusable_menus == NULL){
        endwin();
        exit(EXIT_FAILURE);
    }
    focusable_menus[num_focusable_menus - 1].win = win;
    focusable_menus[num_focusable_menus - 1].menu = menu;
    focusable_menus[num_focusable_menus - 1].is_focused = 0;
}

void update_window_focus(){
    for(int i = 0; i < num_focusable_menus; i++){
        FocusableMenu *menu_item = &focusable_menus[i];

        if(menu_item -> is_focused){
            set_menu_fore(menu_item -> menu, A_REVERSE);
            set_menu_back(menu_item -> menu, A_NORMAL);
            set_menu_mark(menu_item -> menu, " * ");
            
            wattron(menu_item -> win, A_BOLD);
            box(menu_item -> win, 0, 0);
            wattroff(menu_item -> win, A_BOLD);
        }else{
            set_menu_fore(menu_item -> menu, A_REVERSE | A_DIM);
            set_menu_back(menu_item -> menu, A_DIM);
            set_menu_mark(menu_item -> menu, " * ");

            box(menu_item -> win, 0, 0);
        }
        wrefresh(menu_item -> win);
    }
    refresh();
}

void switch_focus(){
    if (num_focusable_menus <= 1) return;

    focusable_menus[current_focus_idx].is_focused = 0;
    current_focus_idx = (current_focus_idx + 1) % num_focusable_menus;
    focusable_menus[current_focus_idx].is_focused = 1;

    update_window_focus();
}

void cleanup_menus(){
    if(focusable_menus != NULL){
        for (int i = 0; i < num_focusable_menus; i++){
            unpost_menu(focusable_menus[i].menu);
            free_menu(focusable_menus[i].menu);
        }
        free(focusable_menus);
        focusable_menus = NULL;
    }
    num_focusable_menus = 0;
    current_focus_idx = 0;
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

void reload_tasks_menu(sqlite3 *db, TasksPane *tasks_pane, const char *where_clause){

    int window_height = src_height - 6;
    int window_width = (src_width - SIDE_BAR_WIDTH) / 2;

    unpost_menu(tasks_pane->menu);

    ITEM **old_items = (ITEM **)menu_items(tasks_pane->menu);
    int old_count = tasks_pane->tasks_struct.task_count;

    int loading_response = load_tasks_fillterd(db, &tasks_pane->tasks_struct, where_clause);
    int tasks_count = tasks_pane->tasks_struct.task_count;

    werase(tasks_pane->win);

    box(tasks_pane->win, 0, 0);

    print_in_middle(tasks_pane->win, 1, 0, window_width, "TASKS");
    mvwaddch(tasks_pane->win, 2, 0, ACS_LTEE);
    mvwhline(tasks_pane->win, 2, 1, ACS_HLINE, window_width - 2);
    mvwaddch(tasks_pane->win, 2, window_width - 1, ACS_RTEE);

    if (loading_response == -1){

        tasks_pane->items = calloc(2, sizeof(*tasks_pane->items));
        tasks_pane->items[0] = new_item("Unable to load tasks from database", NULL);
        tasks_pane->items[1] = NULL;

        set_menu_items(tasks_pane->menu, tasks_pane->items);

    } else if (loading_response == 0){

        tasks_pane->items = calloc(2, sizeof(*tasks_pane->items));


        
    } else {

            tasks_pane->items = calloc(tasks_count+1, sizeof(*tasks_pane->items));

            for (int i = 0; i < tasks_count; i++){
                tasks_pane->items[i] = new_item(tasks_pane->tasks_struct.task_list[i].title, NULL);
                set_item_userptr(tasks_pane->items[i], &tasks_pane->tasks_struct.task_list[i]);
            }
            tasks_pane->items[tasks_count] = NULL;
            
            set_menu_items(tasks_pane->menu, tasks_pane->items);
    }

    


    for(int i = 0; i < old_count; i++){
        free_item(old_items[i]);
    }
    free(old_items);



    post_menu(tasks_pane->menu);
    wrefresh(tasks_pane->win);

}


void create_no_tasks_message(char *buffer, size_t buffer_size, const char *where_clause) {
    if (!where_clause || strcmp(where_clause, "1=1") ){
        snprintf(buffer, buffer_size, "No tasks found");
    } else if (strstr(where_clause, "date")){
        if (strstr(where_clause, "date(due_date, 'unixepoch')=date('now')")){
            snprintf(buffer, buffer_size, "No tasks for today");
        } else if (strstr(where_clause, "date(due_date, 'unixepoch')=date('now','+1 day')")){
            snprintf(buffer, buffer_size, "No tasks for tomorow");
        } else {
            snprintf(buffer, buffer_size, "No over-due tasks");
        }
    }
}


void show_task_details(WINDOW *win ,Task *t) {
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


WINDOW* create_top_bar() {
    WINDOW *top_bar = create_newwin(3, src_width, 0, 0);
    return top_bar;
}


void create_filter_menu(ITEM ***filter_items, MENU **filter_menu, WINDOW **filters_bar_win) {
    *filter_items = (ITEM **)calloc(tasks_filters_count + 1, sizeof(ITEM *));
    for (int i = 0; i < tasks_filters_count; i++) {
        (*filter_items)[i] = new_item(tasks_filters_list[i], NULL);
    }
    (*filter_items)[tasks_filters_count] = NULL;

    *filter_menu = new_menu((ITEM **)*filter_items);
    *filters_bar_win = create_newwin(10, SIDE_BAR_WIDTH, 3, 0);
    keypad(*filters_bar_win, TRUE);
    
    set_menu_win(*filter_menu, *filters_bar_win);
    set_menu_sub(*filter_menu, derwin(*filters_bar_win, 6, SIDE_BAR_WIDTH - 2, 4, 2));
    set_menu_mark(*filter_menu, " * ");
    box(*filters_bar_win, 0, 0);

    print_in_middle(*filters_bar_win, 1, 0, SIDE_BAR_WIDTH, "Filters");
    mvwaddch(*filters_bar_win, 2, 0, ACS_LTEE);
    mvwhline(*filters_bar_win, 2, 1, ACS_HLINE, SIDE_BAR_WIDTH - 2);
    mvwaddch(*filters_bar_win, 2, SIDE_BAR_WIDTH - 1, ACS_RTEE);

    post_menu(*filter_menu);
    wrefresh(*filters_bar_win);
}

void create_tags_menu(sqlite3 *db, ITEM ***tags_items, MENU **tags_menu, WINDOW **tags_bar_win, char ***tags_list, int *tags_count) {
    load_tags(db, tags_list, tags_count);

    *tags_items = (ITEM **)calloc(*tags_count + 1, sizeof(ITEM *));
    for (int i = 0; i < *tags_count; i++) {
        (*tags_items)[i] = new_item((*tags_list)[i], NULL);
    }
    (*tags_items)[*tags_count] = NULL;

    *tags_menu = new_menu((ITEM **)*tags_items);
    *tags_bar_win = create_newwin(src_height - 16, SIDE_BAR_WIDTH, 13, 0);
    keypad(*tags_bar_win, TRUE);
    
    set_menu_win(*tags_menu, *tags_bar_win);
    set_menu_sub(*tags_menu, derwin(*tags_bar_win, src_height - 16 - 4, SIDE_BAR_WIDTH - 4, 4, 2));
    set_menu_format(*tags_menu, 35,1);
    set_menu_mark(*tags_menu, " * ");
    box(*tags_bar_win, 0, 0);

    print_in_middle(*tags_bar_win, 1, 0, SIDE_BAR_WIDTH - 1, "Tags");
    mvwaddch(*tags_bar_win, 2, 0, ACS_LTEE);
    mvwhline(*tags_bar_win, 2, 1, ACS_HLINE, SIDE_BAR_WIDTH - 2);
    mvwaddch(*tags_bar_win, 2, SIDE_BAR_WIDTH - 1, ACS_RTEE);

    post_menu(*tags_menu);
    wrefresh(*tags_bar_win);
}

void create_tasks_menu(sqlite3 *db, TasksPane *tasks_pane) {

    load_tasks(db, &tasks_pane->tasks_struct);

    tasks_pane->items = (ITEM **)calloc(tasks_pane->tasks_struct.task_count + 1, sizeof(ITEM *));
    for (int i = 0; i < tasks_pane->tasks_struct.task_count; i++) {
        tasks_pane->items[i] = new_item(tasks_pane->tasks_struct.task_list[i].title, NULL);
        set_item_userptr(tasks_pane->items[i], &tasks_pane->tasks_struct.task_list[i]);
    }
    tasks_pane->items[tasks_pane->tasks_struct.task_count] = NULL;

    int window_height = src_height - 6;
    int window_width = (src_width - SIDE_BAR_WIDTH) / 2;
    int submenu_height = window_height - 6;
    int submenu_width = window_width - 4;

    tasks_pane->win = create_newwin(window_height, window_width, 3, SIDE_BAR_WIDTH);
    tasks_pane->menu = new_menu((ITEM **)tasks_pane->items);
    keypad(tasks_pane->win, TRUE);
    
    set_menu_win(tasks_pane->menu, tasks_pane->win);
    set_menu_sub(tasks_pane->menu, derwin(tasks_pane->win, submenu_height, submenu_width, 4, 2));
    set_menu_format(tasks_pane->menu, 45, 1);
    set_menu_mark(tasks_pane->menu, " * ");
    box(tasks_pane->win, 0, 0);

    print_in_middle(tasks_pane->win, 1, 0, window_width, "TASKS");
    mvwaddch(tasks_pane->win, 2, 0, ACS_LTEE);
    mvwhline(tasks_pane->win, 2, 1, ACS_HLINE, window_width - 2);
    mvwaddch(tasks_pane->win, 2, window_width - 1, ACS_RTEE);

    post_menu(tasks_pane->menu);
    wrefresh(tasks_pane->win);
}

WINDOW* create_task_details_window() {
    int window_height = src_height - 6;
    int window_width = (src_width - SIDE_BAR_WIDTH) / 2;
    int x_pos = SIDE_BAR_WIDTH + window_width;
    
    WINDOW *task_details_win = create_newwin(window_height, window_width, 3, x_pos);
    box(task_details_win, 0, 0);
    
    print_in_middle(task_details_win, 1, 0, window_width, "Details");
    mvwaddch(task_details_win, 2, 0, ACS_LTEE);
    mvwhline(task_details_win, 2, 1, ACS_HLINE, window_width - 2);
    mvwaddch(task_details_win, 2, window_width - 1, ACS_RTEE);
    
    wrefresh(task_details_win);
    return task_details_win;
}
