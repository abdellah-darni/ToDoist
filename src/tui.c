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

char *tasks_filters_list[] = {
    "All",
    "Today",
    "Tomorrow",
    "Over-Due",
    "Completed"
};

typedef struct {
    char title[256];
    char description[512];
    char due_date[20];  // Format: YYYY-MM-DD HH:MM
    int tag_id;
} TaskFormData;

int tasks_filters_count = 5;



typedef struct _FocusableMenu {
    WINDOW *win;
    MENU *menu;
    int is_focused;
} FocusableMenu;

FocusableMenu *focusable_menus = NULL;
int num_focusable_menus = 0;
int current_focus_idx = 0;

int selected_filter_index = 0;
int selected_tag_index = -1; 

int src_width, src_height;

int tags_count;
char **tags_list = NULL;

void init_tui(sqlite3 *db){

    setlocale(LC_ALL, "");

    ITEM  **filter_items = NULL;
    MENU *filter_menu = NULL;
    WINDOW *filters_bar_win = NULL;
    
    ITEM **tags_items;
    MENU *tags_menu;
    WINDOW *tags_bar_win;


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
    update_menu_highlighting();


    while((ch = getch())!='q'){
        update_time_top_bar(top_bar, 1, src_width);

        if (ch == 9){
            switch_focus(task_details_win);
            continue;
        }

        FocusableMenu *current_menu = &focusable_menus[current_focus_idx];
        c = wgetch(current_menu -> win);

        switch (c)
        {
            case 9:
                switch_focus(task_details_win);
                continue;

            case KEY_UP:
                if (current_menu->menu && item_count(current_menu->menu) > 0) {
                    menu_driver(current_menu -> menu, REQ_UP_ITEM);

                    if (current_focus_idx == 2){
                        show_task_details(task_details_win, (Task *)item_userptr(current_item(current_menu -> menu)));
                    }
                }

                break;

            case KEY_DOWN:
                if (current_menu->menu && item_count(current_menu->menu) > 0) {
                    menu_driver(current_menu -> menu, REQ_DOWN_ITEM);

                    if (current_focus_idx == 2){
                        show_task_details(task_details_win, (Task *)item_userptr(current_item(current_menu -> menu)));
                    }
                }

                break;

            case 10: {
                ITEM *current_item_ptr = current_item(current_menu->menu);
                if (!current_item_ptr) break;
                
                const char *sel = item_name(current_item_ptr);

                if (current_focus_idx == 0){ // Filter menu
                    const char *where;
                    int current_item_index = item_index(current_item_ptr);
                    
                    if      (strcmp(sel, "All")      == 0) where = "1=1";
                    else if (strcmp(sel, "Today")    == 0) where = "date(due_date, 'unixepoch')=date('now')";
                    else if (strcmp(sel, "Tomorrow") == 0) where = "date(due_date, 'unixepoch')=date('now','+1 day')";
                    else if (strcmp(sel, "Over-Due") == 0) where = "date(due_date, 'unixepoch')<date('now')";
                    else where = "t.status=1";

                    mark_selected_filter(current_item_index);
                    
                    reload_tasks_menu(db, &tasks_pane, where);

                    // Switch focus to tasks menu
                    for (int i = 0; i < num_focusable_menus; i++){ 
                        focusable_menus[i].is_focused = 0;
                    }
                    current_focus_idx = 2;
                    focusable_menus[2].is_focused = 1;
                    update_menu_highlighting();

                    // Show details of first task
                    if (tasks_pane.menu && item_count(tasks_pane.menu) > 0) {
                        ITEM *first_task = current_item(tasks_pane.menu);
                        if (first_task) {
                            show_task_details(task_details_win, (Task *)item_userptr(first_task));
                        }
                    }

                } else if (current_focus_idx == 1){ // Tags menu
                    int current_item_index = item_index(current_item_ptr);
                    char where[256];
                    snprintf(where, sizeof(where),"tg.name='%s'", sel);
                    
                    mark_selected_tag(current_item_index);
                    
                    reload_tasks_menu(db, &tasks_pane, where);
                    
                    for (int i = 0; i < num_focusable_menus; i++){ 
                        focusable_menus[i].is_focused = 0;
                    }
                    current_focus_idx = 2;
                    focusable_menus[2].is_focused = 1;
                    update_menu_highlighting();

                    if (tasks_pane.menu && item_count(tasks_pane.menu) > 0) {
                        ITEM *first_task = current_item(tasks_pane.menu);
                        if (first_task) {
                            show_task_details(task_details_win, (Task *)item_userptr(first_task));
                        }
                    }
                }
                
                pos_menu_cursor(current_menu -> menu);
                break;
            }
            case 'a':
            case 'A':
                show_add_task_form(db);
    
                // Refresh all windows after form closes
                update_menu_highlighting();
                
                // Update task details if on tasks menu
                if (current_focus_idx == 2 && focusable_menus[2].menu) {
                    ITEM *current = current_item(focusable_menus[2].menu);
                    if (current) {
                        show_task_details(task_details_win, (Task *)item_userptr(current));
                    }
                }
                break;

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


void switch_focus(WINDOW *task_details_win){
    if (num_focusable_menus <= 1) return;

    focusable_menus[current_focus_idx].is_focused = 0;
    current_focus_idx = (current_focus_idx + 1) % num_focusable_menus;
    focusable_menus[current_focus_idx].is_focused = 1;

    update_menu_highlighting();

    if (current_focus_idx == 2 && focusable_menus[2].menu){
        ITEM *current = current_item(focusable_menus[2].menu);
        if (current){
            show_task_details(task_details_win, (Task *)item_userptr(current));
        }
    }
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

    tasks_pane->items = calloc(tasks_count+1, sizeof(*tasks_pane->items));
    // ToDo : check for mem allocation errors and set a fall back  


    for (int i = 0; i < tasks_count; i++){
        tasks_pane->items[i] = new_item(tasks_pane->tasks_struct.task_list[i].title, NULL);
        set_item_userptr(tasks_pane->items[i], &tasks_pane->tasks_struct.task_list[i]);
    }
    tasks_pane->items[tasks_count] = NULL;
    
    set_menu_items(tasks_pane->menu, tasks_pane->items);

    if (old_items){
        for(int i = 0; i < old_count; i++){
            if (old_items[i]){
                free_item(old_items[i]);
            }
        }
        free(old_items);
    }


    post_menu(tasks_pane->menu);
    wrefresh(tasks_pane->win);

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


void update_menu_highlighting(void){
    for (int i = 0; i < num_focusable_menus; i++){
        FocusableMenu *menu_item = &focusable_menus[i];
        
        if(menu_item->is_focused){
            // Focused menu
            set_menu_fore(menu_item->menu, A_REVERSE | A_BOLD);
            set_menu_back(menu_item->menu, A_NORMAL);
            set_menu_mark(menu_item->menu, " * ");
            
            wattron(menu_item->win, A_BOLD);
            box(menu_item->win, 0, 0);
            wattroff(menu_item->win, A_BOLD);
        } else {
            // Non-focused menu
            set_menu_fore(menu_item->menu, A_DIM);
            set_menu_back(menu_item->menu, A_DIM);
            
            // Show selection indicator for filters/tags even when not focused
            if(i == 0 && selected_filter_index >= 0) {
                // Filter menu show which filter is active
                set_menu_mark(menu_item->menu, " > ");
                // Position cursor on selected filter
                ITEM **items = menu_items(menu_item->menu);
                int item_count_val = item_count(menu_item->menu);
                if (selected_filter_index < item_count_val) {
                    set_current_item(menu_item->menu, items[selected_filter_index]);
                }
            } else if(i == 1 && selected_tag_index >= 0) {
                // Tag menu
                set_menu_mark(menu_item->menu, " > ");
                
                ITEM **items = menu_items(menu_item->menu);
                int item_count_val = item_count(menu_item->menu);
                if (selected_tag_index < item_count_val) {
                    set_current_item(menu_item->menu, items[selected_tag_index]);
                }
            } else {
                set_menu_mark(menu_item->menu, "   ");
            }
            
            box(menu_item->win, 0, 0);
        }
        wrefresh(menu_item->win);
    }
    refresh();
}

void mark_selected_filter(int selected_index) {
    selected_filter_index = selected_index;
    selected_tag_index = -1;
}

void mark_selected_tag(int selected_index) {
    selected_tag_index = selected_index;
    selected_filter_index = -1;
}

// form:

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
    FIELD *field[5];
    FORM *form;
    WINDOW *form_win, *form_subwin;
    PANEL *form_panel;

    form_win = create_form_window();
    keypad(form_win, TRUE);

    char selected_tag[64] = "None"; // the deffault tag is none (no tag) 

    mvwprintw(form_win, 4, 2, "Title:");
    mvwprintw(form_win, 6, 2, "Description: ");
    mvwprintw(form_win, 12, 2, "Due Date:");
    mvwprintw(form_win, 13, 2, "HH:MM DD/MM/YYYY");
    mvwprintw(form_win, 15, 2, "Tag:");
    mvwprintw(form_win, 15, 16, "[%s] Press TAB to select", selected_tag);
    
    mvwprintw(form_win, 22, 2, "ENTER: Next field/Save | ESC: Cancel | ↑↓: Navigate | TAB on Tag: Select");
    
    
    // mvwprintw(form_win, 22, 2, "Press ENTER to save, ESC to cancel, ↑↓ to navigate.");


    form_subwin = derwin(form_win, 14, 52, 4, 16);


    field[0] = new_field(1, 50, 0, 0, 0, 0); //Title
    field[1] = new_field(4, 50, 2, 0, 0, 0); // Descreption
    field[2] = new_field(1, 50, 8, 0, 0, 0); // Due date
    field[3] = new_field(1, 30, 11, 0, 0, 0); // Tag
    field[4] = NULL;

    for(int i = 0; i < 4; i++) {
        set_field_back(field[i], A_UNDERLINE);
        field_opts_off(field[i], O_AUTOSKIP);
    }

    field_opts_off(field[3], O_EDIT);
    // field_opts_off(field[3], O_ACTIVE);
    set_field_buffer(field[3], 0, selected_tag);

    form = new_form(field);
    set_form_win(form, form_win);
    set_form_sub(form, form_subwin);
    post_form(form);

    form_panel = new_panel(form_win);
    top_panel(form_panel);
    
    update_panels();
    doupdate();
    curs_set(1);
    
    int ch;
    int current_field = 0;

    while ((ch = wgetch(form_win)) != 27) {
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
                    char *new_tag = show_tag_menu(form_win, tags_list, tags_count);
                    if (strcmp(new_tag, "-- Add new tag --") == 0){
                        show_add_tag_win(form_win, tags_list, tags_count, db);
                    }
                    // strcpy(selected_tag, new_tag);
                    // set_field_buffer(field[3], 0, selected_tag);
                    // mvwprintw(form_win, 15, 16, "[%-15s] Press TAB to select", selected_tag);
                    // wrefresh(form_win);
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
                    // goto save_form;
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

    // if (ch == 10) {
    //     TaskFormData data = {0};
        
    //     form_driver(form, REQ_VALIDATION);
        
    //     // Extract field buffers (trim whitespace)
    //     char *title_buf = field_buffer(field[0], 0);
    //     char *desc_buf = field_buffer(field[1], 0);
    //     char *date_buf = field_buffer(field[2], 0);
        
    //     // Trim trailing spaces (field buffers are padded)
    //     sscanf(title_buf, "%255[^\n]", data.title);
    //     sscanf(desc_buf, "%511[^\n]", data.description);
    //     sscanf(date_buf, "%19[^\n]", data.due_date);
        
    //     // TODO: Validate and save to database
    //     // insert_task(db, &data);
    // }
    
    curs_set(0);

    // Cleanup
    unpost_form(form);
    free_form(form);
    for(int i = 0; i < 3; i++) {
        free_field(field[i]);
    }

    hide_panel(form_panel);
    del_panel(form_panel);
    delwin(form_subwin);
    destroy_form_window(form_win);
}



char* show_tag_menu(WINDOW *parent_win, char **tags, int tag_count) {
    static char selected_tag[64] = "None";
    
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
                // if (strcmp(selected_tag, "-- Add new tag --") == 0){
                //     // hide_panel(menu_panel);
                //     // unpost_menu(the_menu);
                //     show_add_tag_win(parent_win, tags, tag_count);
                // }
                // // touchwin(menu_win);
                // // wrefresh(menu_win);
                goto exit_menu;
                break;
            case 27:
                strcpy(selected_tag, "None");
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

    return selected_tag;
}

char *show_add_tag_win(WINDOW *parent_win, char **tags, int tag_count, sqlite3 *db){
    char new_tag[31] = "";

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

    char input[64] = "";
    int pos = 0;
    int ch;

    
    curs_set(1);

    while(1){
        

        wmove(add_tag_win, 6, 2);
        for (int i = 0; i < 30; i++) {
            waddch(add_tag_win, ' ');
        }

        mvwprintw(add_tag_win, 6, 2,"%s", input);
        wmove(add_tag_panel, 6, 2 + pos);
        wrefresh(add_tag_win);

        ch = wgetch(add_tag_win);

        switch (ch)
        {
        case 10:
            if (pos > 0){
                strcpy(new_tag, input);
                int exists = is_tag_exist(db, new_tag);
                if (exists == -1){
                    mvwprintw(add_tag_win, 8, 1, "Error checking tag existence!");
                }else if (exists == 0){
                    hide_panel(add_tag_panel);
                    del_panel(add_tag_panel);
                    delwin(add_tag_win);
                
                    return new_tag;
                }else{
                    mvwprintw(add_tag_win, 8, 1, "The tag \"%s\" already exist...", new_tag);
                }
            }
            break;
        case 27:
            hide_panel(add_tag_panel);
            del_panel(add_tag_panel);
            delwin(add_tag_win);

            return NULL;
                
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

    return NULL;
}
